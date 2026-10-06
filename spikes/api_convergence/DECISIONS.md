# Design decisions for the typed route, by example

The typed route ([README.md](README.md)) measures well. What it still needs is seven decisions that measurement
cannot make. This document gives each one a concrete example: the code a user would write, what happens today in
the spike, the options with their code, and which way I lean and why.

Every compiler message and program output quoted here was produced by compiling and running the snippet shown
against this branch (MSVC 19.51 and clang-cl 22; messages trimmed to their first lines). Costs are from
[`results/`](results/). "The spike" means [`include/sub0pub_spike/topology.hpp`](include/sub0pub_spike/topology.hpp).

The running example is the station in [`examples/typed_route`](examples/typed_route/):

```cpp
struct Reading { int celsius; };

class Display final : public sub0::spike::Subscribe<Reading>
{
public:
    void receive(const Reading& reading) noexcept;
};

class Thermometer final : public sub0::spike::Publish<Reading>
{
public:
    void measure(int celsius) noexcept { sub0::spike::publish(*this, Reading{celsius}); }
};
```

| # | Decision | I lean towards |
|---|---|---|
| [1](#1-one-topology-per-message-type) | Is one topology per message type, program-wide, the right boundary? | Yes; `Tagged` and explicit wiring cover the rest |
| [2](#2-where-the-topology-lives-and-what-happens-when-a-unit-misses-it) | Where does the topology line live, and how is a unit that misses it caught? | A project header set by the build system, plus the debug check added here |
| [3](#3-what-a-publisher-has-to-see) | Must a publisher's translation unit see the receivers? | Inline by default; an out-of-line route as the documented way to decouple |
| [4](#4-override) | `receive()` without `override`, or keep it virtual in static builds? | Without `override`; the two checks it gave are replaced |
| [5](#5-semantics-that-still-differ) | Which differences in meaning are documented, and which are closed? | Document order and the closed set; converge `filter()`; leave cancellation separate |
| [6](#6-staticfirst) | Ship `StaticFirst` (bound receivers, then the broker)? | Not in the first version |
| [7](#7-header-layering) | How does the broker area follow a topology without including wiring? | By detecting a member of the configuration: no include is needed |

---

## 1. One topology per message type

**The question.** `SUB0PUB_CONFIGURE(Reading, StaticTo<&display, &audit>)` routes *every* `Reading` in the program
to those two objects. That is the granularity of the runtime broker's default storage (one table per type), but it
means two stations in one program cannot wire `Reading` differently.

**Example.** A second station is added:

```cpp
Display displayA, displayB;
SUB0PUB_CONFIGURE(Reading, sub0::spike::StaticTo<&displayA>);   // there can be only one

Thermometer a, b;
a.measure(20);   // -> displayA
b.measure(30);   // -> displayA as well; displayB is never called
```

`displayB` is a subscriber of `Reading` that the topology does not list, so a debug build stops at its constructor
([decision 5](#5-semantics-that-still-differ)); a release build delivers nothing to it.

**Options.**

*A. Accept it, and give each station its own message type with `Tagged`.* This compiles and behaves today:

```cpp
template<class Station> class Display;
struct StationA;
struct StationB;
extern Display<StationA> displayA;
extern Display<StationB> displayB;

struct StationA { using sub0_config = sub0::config<sub0::spike::StaticTo<&displayA>>; };
struct StationB { using sub0_config = sub0::config<sub0::spike::StaticTo<&displayB>>; };
template<class Station> using ReadingOf = sub0::Tagged<Reading, Station>;

template<class Station>
class Display final : public sub0::spike::Subscribe<ReadingOf<Station>>
{
public:
    void receive(const ReadingOf<Station>& reading) noexcept { last = reading.value.celsius; ++count; }
    int last = 0;
    unsigned count = 0;
};
```

```text
A: 1 readings, last 20; B: 2 readings, last 31
```

The price is that the participants become templates on the station, and the payload is reached through `.value`.

*B. Accept it, and use explicit wiring for the multi-instance case.* `wire(displayB)` or `StaticWiring<&displayB>`
with a `Thermometer<Bus>`: today's static API, unchanged. The price is that this station's publisher is written
in the other style; the typed route's `Thermometer` cannot be handed a wiring.

*C. Allow a topology per publisher instance.* That is a wiring object held by the publisher, which is option B
under another name, and it would put a member back into `Publish<T>`.

**Lean: accept the boundary (A and B as the documented escapes).** It mirrors the rule the broker already has
(`Scoped` + `Domain` when one table per type is not enough), and most programs have one of each message type.

---

## 2. Where the topology lives, and what happens when a unit misses it

**The question.** A topology is a per-type configuration, so every translation unit that publishes or subscribes
the type must see it (the contract docs/DESIGN.md records as K6). For the existing options a violation is undefined
behaviour that usually goes unnoticed. For a topology it has a visible consequence, and I measured it.

**Example.** `main.cpp` includes the project's `topology.hpp`; `legacy_sensor.cpp` was never updated and does not:

```cpp
// topology.hpp
class Display;
extern Display display;
SUB0PUB_CONFIGURE(Reading, sub0::spike::StaticTo<&display>);
#include "display.hpp"

// main.cpp: includes topology.hpp
thermometer.measure(20);    // a direct call to display.receive()
legacyMeasure(21);

// legacy_sensor.cpp: does not; for this unit Reading is brokered
void legacyMeasure(int celsius) noexcept
{
    LegacySensor sensor;                      // derives from Publish<Reading>
    sensor.measure(celsius);                  // publishes to a broker table nobody subscribed to
}
```

It compiles and links without a warning. In a release build:

```text
display received 1 of 2
```

The existing `SUB0PUB_CHECK_CONFIG` debug check does not see it either (the same output with the check on),
because the static side never constructs a broker for it to compare.

**What I added.** The spike now records, in debug builds, the topology each unit publishes a type with, and
reports the first disagreement. With it, the same program in a debug build stops at `legacyMeasure(21)`:

```text
Assertion failed: sub0pub: a message type was published with different topologies in different translation units
```

It is compiled out of release builds (`SUB0PUB_CHECK_CONFIG` is false), so it costs the static path nothing. One
detail is worth knowing if this moves in-library: the first version of the check did not fire. The two units each
instantiated the same inline function with a different body, and the linker kept one copy. The topology has to be
a template parameter of the checking function, as the configuration is of the existing `checkConfig`.

**Options for where the line lives.**

*A. A project header given to every unit by the build system.* Every unit then agrees by construction. Two ways:

- a compiler force-include (`/FIstation_topology.hpp`, or `-include station_topology.hpp`), which needs nothing
  from the library (not exercised here: the examples include the header explicitly);
- a library include point, as `SUB0PUB_CONFIG_HEADER` is for the project default:

  ```cmake
  target_compile_definitions(station PRIVATE SUB0PUB_TOPOLOGY_HEADER="station_topology.hpp")
  ```

  The existing include point cannot be reused: `config.hpp` reads it before `SUB0PUB_CONFIGURE` and
  `sub0::config` exist, so a topology header needs a second one, after the configuration vocabulary is complete.

*B. Next to the message type*, as the member alias or the ADL declaration:

```cpp
struct Reading
{
    int celsius;
    using sub0_config = sub0::config<sub0::spike::StaticTo<&display, &audit>>;
};
```

Also agrees by construction, but the message now names its receivers. `wire.hpp` states the opposite principle
("Messages never list receivers") and decision D1 rejected a central registry for the same dependency reason.

*C. An explicit include in each source*, which is what the examples here do for want of the include point. This is
the form that fails as shown above.

**Options for catching a miss.**

| Option | Catches | Cost |
|---|---|---|
| The debug check above | the first publish from a disagreeing unit, at run time, in debug builds | none in release |
| Link-time detection: each unit references a symbol named after the topology it resolved, defined once | every disagreement, at link time | not spiked; the reference must survive optimisation, so likely a pointer of RAM per unit, or debug-only |
| Nothing beyond the build contract | nothing | none |

**Lean: A, plus the debug check.** Link-time detection is worth a spike of its own only if a release-build miss is
judged unacceptable, since option A makes the miss hard to construct.

---

## 3. What a publisher has to see

**The question.** A direct call can only be inlined into a publisher that can see the receiver's definition. So a
publisher of a statically routed type depends, at compile time, on every receiver of that type.

**Example.** The station's topology header ends by including the receivers, so that `thermometer.cpp` gets them:

```cpp
SUB0PUB_CONFIGURE(Reading, sub0::spike::StaticTo<&display, &audit>);

#include "audit.hpp"
#include "display.hpp"
```

Leave those two includes out and the publisher's unit does not compile. This is deliberate: without the check the
compiler would find no `receive()` on an incomplete type and silently deliver nothing.

```text
topology.hpp: error C2027: use of undefined type 'Audit'
topology.hpp: error: invalid application of 'sizeof' to an incomplete type 'Audit'
```

**Options.**

*A. Inline (the spike's form above).* Equal to hand-written code, including across translation units: in the
`cross_file` case, with `receive()` defined in another unit, it is 39 / 53 / 53 instructions per publication
(MSVC / clang-cl / icx), the same as hand-written direct calls. The price is the include dependency: touching a
receiver's header recompiles every publisher of that type.

*B. An out-of-line route.* The type is routed to one small object whose `receive()` is defined in the unit that
composes the application. This already works with the spike as it is (variant `route_static_out_of_line`):

```cpp
// what a publisher sees
struct ReadingRoute { void receive(const Reading& reading) noexcept; };
inline ReadingRoute readingRoute;
SUB0PUB_CONFIGURE(Reading, sub0::spike::StaticTo<&readingRoute>);

// station.cpp: the one place that lists the receivers
void ReadingRoute::receive(const Reading& reading) noexcept
{
    sub0::StaticWiring<&display, &audit>::publish(reading);
}
```

| `cross_file`, instructions per publication | MSVC | MSVC `/LTCG` | clang-cl | icx |
|---|---|---|---|---|
| hand-written, and option A | 39 | 19 | 53 | 53 |
| option B | 46 (+7) | 19 (+0) | 57 (+4) | 57 (+4) |

One call that link-time optimisation removes. Two things are not solved by doing it by hand: the receivers still
derive from `Subscribe<Reading>` but are no longer in the type's `StaticTo` list, so the debug "not listed"
assertion would fire for them; and nothing ties the route to the type. An in-library form would be an option of
its own, say `StaticVia<&route>`, that knows the route stands for the receivers.

*C. Rely on LTO and keep one form.* Then a build without LTO silently pays for every static type.

**Lean: A as the default, B as a documented option for types whose receivers change often.** B needs the small
in-library design noted above before it is more than a pattern.

---

## 4. `override`

**The question.** `Subscribe<Reading>` has a virtual `receive` when `Reading` is brokered and no virtual at all
when it is `StaticTo`. A receiver that is to work in both cannot say `override`.

**What `override` protects against, and whether that is lost.** The mistake it catches is a signature that does
not match:

```cpp
class Display final : public sub0::spike::Subscribe<Reading>
{
public:
    void receive(Reading& reading) noexcept;     // not const: this does not override
};
```

*In a dynamic build it is still a compile error*, because the base's `receive` is pure virtual:

```text
error C2259: 'Display': cannot instantiate abstract class
note: 'void sub0::detail::SubscriberInterface<Data,false>::receive(const Data &) noexcept': is abstract

error: variable type 'Display' is an abstract class
warning: 'Display::receive' hides overloaded virtual function [-Woverloaded-virtual]
```

*In a static build it was not.* Capability routing found no `receive(const Reading&)`, skipped the receiver, and
the program ran:

```text
display received 0 of 1
```

That is limitation K14, and it would have made a promotion to `StaticTo` able to drop a receiver silently. **The
spike now rejects it**: a receiver that is listed in a type's `StaticTo` and derives from `Subscribe` of that type
must be able to receive it.

```text
error C2338: static assertion failed: 'sub0pub: a receiver listed in StaticTo<> derives from Subscribe<Data>
             but has no receive(const Data&)'
```

So both builds now reject the mistake without `override`. What remains is style: a project that enables
`-Wsuggest-override` gets, in dynamic builds,

```text
warning: 'receive' overrides a member function but is not marked 'override' [-Wsuggest-override]
```

(`/W4` and `-Wall -Wextra -Wpedantic` are silent.)

**Options.**

*A. No `override` (the spike's default).* One edit per receiver when existing broker code is first made promotable:
delete the keyword. Zero cost in every build.

*B. Keep `receive` virtual in static builds*, so existing `override` receivers compile untouched
(`SUB0PUB_SPIKE_STATIC_VIRTUAL`). The publish path is unchanged when the receiver is `final`. The cost is a vptr
per subscribed type and the retained vtables: for the station's three receivers, RAM +384 / +144 / +128 B and
text +936 / +239 / +304 B (MSVC / clang-cl / icx). A receiver that is *not* `final` would be called through its
vtable unless the compiler can prove its type; that case is not measured.

*C. A macro*, `void receive(const Reading&) noexcept SUB0PUB_OVERRIDE;`. It cannot work: whether the keyword is
valid depends on the message type, which a macro cannot see.

**Lean: A.** B's only remaining purpose is a migration with no edits at all, and it gives up the "no RAM" property
that is the point of static storage on small targets. It could be offered as a project-wide opt-in for exactly
that migration.

---

## 5. Semantics that still differ

These are differences in meaning between a brokered type and a wired one. Converging the spelling must not hide
them. For each: is it documented, or closed?

### 5a. Delivery order

```cpp
Audit audit;          // constructed first
Display display;
SUB0PUB_CONFIGURE(Reading, sub0::spike::StaticTo<&display, &audit>);   // static build only
```

```text
dynamic:  audit display        (registration order = construction order)
static:   display audit        (the order of the list)
```

A program whose receivers depend on each other's side effects changes behaviour when promoted. **Lean: document
it**, and recommend listing receivers in construction order when order matters. The static order is at least
visible in one line; the dynamic one is spread over the program.

### 5b. The set of receivers is closed

```cpp
Display display;      // listed in StaticTo<&display>
Display spare;        // not listed
```

Release build: `display 1 spare 0`. Debug build, at `spare`'s constructor:

```text
Assertion failed: sub0pub: this subscriber is not listed in its type's StaticTo<>
```

The awkward case is a unit test that constructs one `Display` on its own while the project's static topology is in
force: it trips the assertion although nothing is wrong. **Options:** keep the assertion and have such tests build
without the topology header (they then exercise the same class as a broker subscriber); make it a hook like
`SUB0PUB_REENTRANT_VIOLATION` so a test build can count instead of abort; or drop it. **Lean: keep it, as an
overridable hook.** The silent alternative is the worst failure this design has.

### 5c. Features that exist only on the broker

Using one on a statically routed type is a compile error, which is the behaviour I would keep: the compiler lists
what a promotion has to resolve.

```cpp
display.disconnect();
```

```text
error C2039: 'disconnect': is not a member of 'Display'
```

The same is true of `trySubscribe()`, `isSubscribed()` and `sub0::cancel()`. The message could be better: an
in-library form can declare these members deleted with a reason, so the error says "Reading is StaticTo: its
receivers are fixed".

`Route`, `Domain` and publish reports are not covered by the spike. `Route` derives from `sub0::Subscribe`
directly, not from the spike's alias, so here it would compile for a `StaticTo` type, register with a broker that
is never published to, and send nothing. In-library, where `Subscribe<T>` itself follows the topology, it has to
be made an error like the others.

### 5d. Cancellation

The two models stop a publication in different ways, and the typed route converges neither:

```cpp
// broker: needs a publish context (SUB0PUB_CANCEL or sub0::ThreadLocalContext)
void receive(const Reading& reading) noexcept override { if (reading.celsius > 90) cancel(); }

// wiring: the receiver returns bool, and the publisher calls publishCancelable
bool receive(const Reading& reading) noexcept { return reading.celsius <= 90; }
```

On a `StaticTo` type the first form does not compile (`cancel()` needs the broker), and the second is silently
ignored, because the spike's `publish()` calls `StaticWiring::publish`, not `publishCancelable` (limitation K19).

**Options:** (A) leave cancellation out of the converged API and document it as a reason to stay on one model for
that type; (B) converge on the `bool` form, which means the broker's virtual `receive` returns `bool` for types
that opt in, a breaking change to `Subscribe<T>`; (C) converge on `cancel()`, which decision D5 already rejected
for static wiring on measured cost (thread-local state, +14 path instructions on Cortex-M33). **Lean: A now**, and
make the ignored `bool` a compile error on a `StaticTo` type so it cannot be missed.

### 5e. `filter()`

```cpp
class Display final : public sub0::spike::Subscribe<Reading>
{
public:
    bool filter(const Reading& reading) noexcept { return reading.celsius >= 0; }
    void receive(const Reading&) noexcept;
};
```

| Configuration of `Reading` | Result |
|---|---|
| default (brokered) | `error C2555: 'Display::filter': overriding virtual function return type differs` : filters are opt-in |
| `sub0::Filter` (brokered) | works: `-5` is filtered |
| `StaticTo<&display>` | works, although the type never opted in |
| `sub0::Filter` + `StaticTo<&display>` | works |

So it converges when the type opts in, and the static build is more permissive than the dynamic one. **Lean: close
it**: on a `StaticTo` type, honour `filter()` only if the type's configuration has `Filter`, and reject a receiver
that declares one otherwise, as the broker does. Then the same source is accepted or rejected in both builds.

---

## 6. `StaticFirst`

**The question.** `StaticFirst<&display>` calls the display directly and then publishes through the broker, so
other receivers can still subscribe at run time. It is the "promote one known receiver" step. Is it worth shipping?

**Example.** The `bridged` station: one line, and the audit log keeps subscribing as before.

```cpp
SUB0PUB_CONFIGURE(Reading, sub0::spike::StaticFirst<&display>);
```

**What it costs**, against `StaticTo` with the same receivers and nobody subscribed at run time:

| | MSVC | clang-cl | icx |
|---|---|---|---|
| `one_receiver`, instructions per publication | +11 | +9 | +9 |
| `station` (two types), instructions per publication | +21 | +18 | +18 |
| `batch`, a burst of 64: instructions | 732 against 205 | 149 against 136 | 739 against 250 |
| `batch`: is the burst still vectorised? | no | yes | no |

It also keeps the broker's table, vtables and registration code in the image (RAM +350 to +420 B for one type),
and it is the one mode in which a bound receiver's `receive()` stays virtual, so a receiver that is not `final`
may be called through its vtable.

**The alternative it competes with** is two types, or simply listing the receiver:

```cpp
SUB0PUB_CONFIGURE(Reading, sub0::spike::StaticTo<&display, &audit>);   // if audit is in fact always there
```

and, when the open set is real, leaving the type dynamic.

**Lean: not in the first version.** `StaticTo` per type covers the promotions that pay, and `StaticFirst` has the
most surprising cost profile of anything here. If it ships later, it should decide before registering a bound
receiver (the spike registers and then disconnects, which costs 182 against 56 setup instructions on MSVC), and
its documentation should say plainly not to use it on a burst path.

---

## 7. Header layering

**The question.** STYLE_GUIDE.md: "An area's entry header must not reach another area; bridges between areas get
their own header." `sub0::publish()` and `sub0::Subscribe<T>` are in the broker area, and a statically routed
publish is wiring.

**It does not need an include.** The spike's `publish()` finds the topology by asking the type's configuration
whether it has one, and then calls a member of it:

```cpp
template<class Config>
    requires requires { typename Config::topology; }
struct TopologyOf<Config> { using type = typename Config::topology; };

if constexpr (Topology::cHasStatic)
    Topology::publish(data);            // a dependent call: resolved where publish() is instantiated
```

Nothing there names `StaticWiring`. The broker headers would only need: "if this type's configuration has a
`topology`, `Subscribe<T>` is an empty class and `publish()` calls `topology::publish`". The header that *defines*
`StaticTo` is the one that includes wiring, and that is a bridge header in the sense the rule intends, like the
existing `sub0pub/wiring/broker_port.hpp`:

```text
sub0pub/config.hpp            options and resolution; knows nothing of topologies
sub0pub/broker/*.hpp          Subscribe / Publish / publish: follow Config::topology if it exists
sub0pub/wiring/*.hpp          StaticWiring: unchanged
sub0pub/wiring/static_to.hpp  StaticTo<&...> : the bridge; includes wiring, is included by the umbrella
                              and by a project's topology header
```

**Options:** (A) the above; (B) let `broker/publish.hpp` include `wiring/capability.hpp` and amend the rule;
(C) keep `sub0::publish` broker-only and give the converged entry points new names in a bridge header, as the
spike does with `sub0::spike::`. C keeps the areas apart at the price of the main result: existing broker code
would no longer be the converged spelling.

**Lean: A.** One consequence to check when it is built: `tests/headers` compiles each header alone, and
`broker/*.hpp` must still pass without the bridge present, which the duck-typed detection gives.

---

## What changed in the spike while writing this

Three checks were added to `topology.hpp` because the examples above exposed the gaps. None changes generated code
in a release build; the recorded results were regenerated afterwards.

| Check | Catches | When |
|---|---|---|
| a listed receiver that derives from `Subscribe<T>` must have `receive(const T&)` | a mismatched signature silently skipped (decision 4) | compile time |
| the topology a unit publishes a type with must match every other unit's | a unit that missed the topology header (decision 2) | debug builds, at the publish |
| (already there) a subscriber must be listed in its type's `StaticTo` | a receiver left out of the list (decision 5b) | debug builds, at construction |
