# Design decisions for the typed route, by example

The typed route ([README.md](README.md)) measures well. What it needed beyond measurement was a set of design
decisions. This document gives each one a concrete example (the code a user writes, what the spike does with it,
the options and their costs) and records where it now stands.

**Status as of 2026-10-07**, after the owner's review. The [audit](#6-staticfirst-and-the-audit) added since then
is one recording that answers, from a run, several of the questions below; each section says where. "Decided" items are settled for the first in-library
version; "deferred" ones are deliberately left open until that version exists.

| # | Decision | Status | Direction |
|---|---|---|---|
| [1](#1-one-topology-per-message-type) | One topology per message type, program-wide | **Decided** | The type decides. `Tagged` makes a new type where the configuration must vary |
| [2](#2-where-the-configuration-lives-and-publishing-to-nobody) | Where the configuration lives; a unit that misses it | **Decided** | Next to the type by default. `SUB0PUB_CONFIGURE` only for types that cannot be edited. Publishing to no receiver is a failure by default |
| [3](#3-what-a-publisher-has-to-see) | What a publisher has to see | Deferred | Inline is the normal form; the out-of-line route is a pattern users may apply. Revisit on a production sample |
| [4](#4-override) | `receive()` without `override` | **Decided** | No `override` |
| [5a](#5a-delivery-order) | Delivery order differs | **Decided** | Document it, with an example |
| [5b](#5b-the-set-of-receivers-is-closed) | A receiver left out of the list | **Direction set** | Safe by default with an opt-out; publishers can ask how many receivers a type has; the audit reports it |
| [5c](#5c-features-that-exist-only-on-the-broker) | Broker-only members on a wired type | Deferred | Some may get a static meaning (`isSubscribed()` as a wiring check) |
| [5d](#5d-cancellation) | Cancellation | Deferred | Until after the first convergence API lands |
| [5e](#5e-filter) | `filter()` | **Decided** | An opt-in per type in both models |
| [6](#6-staticfirst-and-the-audit) | `StaticFirst`; the audit | **Direction set** | `StaticFirst` is a user convenience. One audit of a run reports unheard publications, unused receivers and the wiring a type could be given |
| [7](#7-header-layering) | Header layering | Deferred | No hard decision now |
| [8](#8-configuring-a-type-without-the-preprocessor) | Configuring without the preprocessor | Deferred | No hard decision now |

Every compiler message and program output quoted here was produced by compiling and running the snippet shown
against this branch (MSVC 19.51 and clang-cl 22; messages trimmed to their first lines). Costs are from
[`results/`](results/); where a table cites a case, the measured source is that case's variant, which may spell
the same configuration with `SUB0PUB_CONFIGURE`. "The spike" means [`include/sub0pub_spike/`](include/sub0pub_spike/).

The running example is the station in [`examples/typed_route`](examples/typed_route/):

```cpp
struct Reading
{
    int celsius;
    using sub0_config = sub0::config<sub0::spike::StaticTo<&display, &audit>>;   // omit, and Reading is brokered
};

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

---

## 1. One topology per message type

**Decided: the type decides.** A message type has one topology for the whole program, as it has one subscription
table under the broker's default storage. Where the configuration has to vary, `Tagged` makes a distinct type.

**Example.** A second station cannot route `Reading` differently:

```cpp
Display displayA, displayB;
struct Reading { int celsius; using sub0_config = sub0::config<sub0::spike::StaticTo<&displayA>>; };

Thermometer a, b;
a.measure(20);   // -> displayA
b.measure(30);   // -> displayA as well; displayB is never called (and a debug build says so: 5b)
```

With `Tagged`, each station has its own type and its own topology. This compiles and runs today:

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

The participants become templates on the station, and the payload is reached through `.value`. Explicit wiring
(`wire`, `StaticWiring` with a `Thermometer<Bus>`) remains for a publisher that must be handed its output.

**Noted for later.** A topology chosen by the *publisher* could carry weight once a publisher in a hot loop needs
queued or asynchronous fan-out: that is a property of the sending side, not of the type. Nothing here depends on
it, and nothing here rules it out; it is the first thing to reconsider when queued delivery is designed.

---

## 2. Where the configuration lives, and publishing to nobody

**Decided:**

- **Configuration next to the type is the default**: the member alias, or the declaration found by
  argument-dependent lookup. Nothing is forced on the build system and no extra header has to be included.
- **`SUB0PUB_CONFIGURE` is for a type that cannot be edited**, and is documented with its limitation.
- **A publication that reaches no receiver is a reported failure by default**, with an explicit way to allow it.

### The three spellings

Yes: `SUB0PUB_CONFIGURE` exists for a type whose definition is not yours to change (`config.hpp`: "Configure a
Data type you cannot modify"). The member alias and the ADL declaration give exactly the same result; they are
more intrusive only in that they sit in the type's own header. That is also their advantage: the configuration is
part of the type, so every translation unit that can name the type agrees on it.

```cpp
struct Reading                                                         // member alias
{
    int celsius;
    using sub0_config = sub0::config<sub0::spike::StaticTo<&display, &audit>>;
};

struct Reading { int celsius; };                                       // ADL declaration, beside the type
sub0::config<sub0::spike::StaticTo<&display, &audit>> sub0_config(Reading*);

#include "vendor/messages.hpp"                                         // a type that is not yours
SUB0PUB_CONFIGURE(Reading, sub0::spike::StaticTo<&display, &audit>);
```

The station example now uses each where it belongs: the member alias (`static`, `bridged`), the ADL declaration
(`hot_path`) and, for a header that stands for someone else's, `SUB0PUB_CONFIGURE` (`foreign`).

Two things to know about the default:

- The ADL declaration must be in the type's own namespace and precede the type's first use. Anywhere else it is
  ignored without a diagnostic. Written beside the type it cannot go wrong; that is the only place to put it.
- A message type that names its receivers depends on their *declarations* (`class Display; extern Display display;`),
  not their definitions. This is the trade made by putting the topology with the type; `wire.hpp`'s principle that
  "messages never list receivers" still holds for the explicit wiring API, where it was stated.

### The limitation of `SUB0PUB_CONFIGURE`

Because the configuration is no longer part of the type, a translation unit can see the type without it:

```cpp
// topology.hpp
#include "vendor/messages.hpp"
SUB0PUB_CONFIGURE(Reading, sub0::spike::StaticTo<&display>);

// main.cpp: includes topology.hpp
thermometer.measure(20);    // a direct call to display.receive()
legacyMeasure(21);

// legacy_sensor.cpp: includes vendor/messages.hpp only; for this unit Reading is brokered
void legacyMeasure(int celsius) noexcept
{
    LegacySensor sensor;                      // derives from Publish<Reading>
    sensor.measure(celsius);                  // publishes to a broker table nobody subscribed to
}
```

It compiles and links without a warning, and in a release build the message is lost:

```text
display received 1 of 2
```

The existing `SUB0PUB_CHECK_CONFIG` debug check does not see it, because the static side never constructs a broker
for it to compare. Two things in the spike now catch it. The debug-build topology check reports the first
disagreeing publication. And the rule below catches it in a release build as well.

### Publishing to nobody is a failure by default

**What the rule is.** `publish()` of a type that currently has no receiver is reported, unless the type says that
is expected. A call site that wants to decide for itself asks first, or uses a publish that returns the count.

```cpp
struct Diagnostics                                           // receivers are optional: a plug-in, a debug console
{
    int code;
    using sub0_config = sub0::config<sub0::spike::AllowNoReceivers>;
};

if (sub0::spike::tryPublish(*this, Reading{celsius}) == 0U)  // the call site decides
    log("no display attached yet");

if (sub0::spike::receiverCount<Reading>() != 0U)             // or asks first
    sub0::spike::publish(*this, buildExpensiveReading());
```

**How it works in each model, and what it costs.**

| | When it is decided | Cost |
|---|---|---|
| `StaticTo<>` type | compile time: the list is a constant | none |
| Brokered type | at each publication: is the table empty? | none on a publication that has receivers (below) |

For a wired type, a list in which no receiver can take the message does not compile:

```text
error C2338: static assertion failed: 'sub0pub: no receiver listed in this type's StaticTo<> can receive it, so the
             publication would reach nobody; list a receiver, or configure the type with AllowNoReceivers'
```

`StaticTo<>` with `AllowNoReceivers` compiles to nothing, which is a deliberate way to compile a message out, and
`receiverCount<Reading>()` is a constant that a `static_assert` can test.

For a brokered type the check belongs in the broker's publish. The library broker is untouched in this branch, so
the spike measures it with a stand-in broker of its own (`no_receivers.hpp`, through the public
`Implementation<>` option) built in two forms that differ only in the check:

| Instructions per publication (MSVC / clang-cl / icx) | one receiver | station: two types, four deliveries |
|---|---|---|
| library broker | 34 / 40 / 40 | 83 / 103 / 103 |
| stand-in, no check | 33 / 41 / 41 | 80 / 107 / 107 |
| stand-in, reporting an empty publication | 33 / 41 / 41 | 80 / 107 / 107 |

The check replaces the dispatch loop's own entry test, so a publication with receivers executes the same
instructions with or without it, on all three compilers. It adds 16 to 132 bytes of image for the cold path. (My
first version cost MSVC three instructions per publication; writing the loop as test-then-do-while removed them.)

Behaviour, from a small program:

```text
with a display: delivered 1, receiverCount 1
display gone: receiverCount 0, tryPublish reached 0
sub0pub: a message was published and no receiver is subscribed to its type          <- publish(): reported
```

and, with `AllowNoReceivers` on the type, the last line is instead `publish to nobody returned normally`.

**How it aligns with the rest.**

- *It closes the `SUB0PUB_CONFIGURE` limitation in release builds.* A unit that missed the configuration publishes
  into an empty table, which is now the reported case. The two-file program above, with the rule as the project
  default, stops at the stray publication instead of losing the message:

  ```text
  display received 1; now the unit that missed the topology publishes
  sub0pub: a message was published and no receiver is subscribed to its type
  ```

- *It gives dynamic linkage an honest spelling.* A type whose receivers arrive with a module loaded at run time is
  exactly a type for which "nobody yet" is normal: it says so with `AllowNoReceivers`, or its publishers use
  `tryPublish()` and act on the count.
- *It does not replace the "not listed" check of 5b.* If a wired type has three receivers and a fourth was left out
  of the list, publications still reach three; only the per-receiver check sees the fourth.
- *It does not see a mis-route that still reaches somebody*, for example a `StaticFirst` type whose stray unit
  publishes into a broker that other subscribers did join.

**What adopting it changes, and one point to confirm.**

- It is a behaviour change. Today a publication with no subscriber is a silent no-op, and it is not rare: a
  publisher that starts before its subscribers, a last publication after they are destroyed, a diagnostic stream
  nobody is watching. The baseline's "publish, 0 subscribers" scenario and the `zero_receivers` collapse case
  measure exactly that. Each such type needs `AllowNoReceivers`, and `MIGRATION.md` needs the entry.
- In-library it needs three small things in the broker: the check in `BrokerImpl::publish`, a way to read the
  receiver count, and a publish that returns it. The spike has all three, in the stand-in only.
- **To confirm: what "reported" does outside an audit build.** In an [audit build](#6-staticfirst-and-the-audit)
  an empty publication is recorded against the publisher that made it, printed at exit and counted for a test:
  nothing aborts. That gives the rule a place to report to, and it is where I would now put the default. What is
  left to choose is the behaviour of an ordinary build. The spike's in-publish check follows the library's other
  contract violations (a hook whose default asserts in a debug build and aborts in a release build), and aborting a
  release build because a subscriber was constructed late is severe. With the audit available, my lean is: keep
  the check and its hook for projects that want to stop at the first one, and make its default a debug-build
  assertion only, as `SUB0PUB_REENTRANT_CHECK` is. The check itself is free either way.

---

## 3. What a publisher has to see

**Deferred.** The inline form is the normal one: it is what the compiler is told, explicitly, and it is equal to
hand-written code. The out-of-line route moves the dependency rather than removing it, but it is a workable
decoupling mechanism that users can apply themselves. Link-time optimisation is the real fix. This is to be
revisited when the typed route is integrated into a production-style sample.

**Example.** A publisher of a statically routed type must see its receivers' definitions, which is why the
station's types header ends by including them:

```cpp
struct Reading { int celsius; using sub0_config = sub0::config<sub0::spike::StaticTo<&display, &audit>>; };

#include "audit.hpp"
#include "display.hpp"
```

Leave those includes out and the publisher's unit does not compile; without the check the compiler would find no
`receive()` on an incomplete type and silently deliver nothing.

```text
topology.hpp: error C2027: use of undefined type 'Audit'
topology.hpp: error: invalid application of 'sizeof' to an incomplete type 'Audit'
```

**The two forms, measured** (case `cross_file`: `receive()` defined in another translation unit):

```cpp
// B: what a publisher sees
struct ReadingRoute { void receive(const Reading& reading) noexcept; };
inline ReadingRoute readingRoute;
struct Reading { int celsius; using sub0_config = sub0::config<sub0::spike::StaticTo<&readingRoute>>; };

// B: station.cpp, the one place that lists the receivers
void ReadingRoute::receive(const Reading& reading) noexcept
{
    sub0::StaticWiring<&display, &audit>::publish(reading);
}
```

| Instructions per publication | MSVC | MSVC `/LTCG` | clang-cl | icx |
|---|---|---|---|---|
| hand-written, and A (inline) | 39 | 19 | 53 | 53 |
| B (out-of-line route) | 46 (+7) | 19 (+0) | 57 (+4) | 57 (+4) |

Done by hand, B leaves two loose ends for an in-library form: the receivers behind the route derive from
`Subscribe<Reading>` but are not in the type's list, so the "not listed" check would fire for them; and nothing
ties the route to the type.

---

## 4. `override`

**Decided: `receive()` is declared without `override`.**

`Subscribe<Reading>` has a virtual `receive` when `Reading` is brokered and none when it is `StaticTo`, so a
receiver that works in both cannot say `override`. The mistake `override` guarded against is a signature that does
not match:

```cpp
class Display final : public sub0::spike::Subscribe<Reading>
{
public:
    void receive(Reading& reading) noexcept;     // not const: this does not override
};
```

It is a compile error in both builds without the keyword. Brokered, because the base's `receive` is pure virtual:

```text
error C2259: 'Display': cannot instantiate abstract class
error: variable type 'Display' is an abstract class
```

Wired, because the spike requires a listed receiver that derives from `Subscribe` of the type to be able to
receive it. (Before that check it compiled and delivered `0 of 1`: limitation K14.)

```text
error C2338: static assertion failed: 'sub0pub: a receiver listed in StaticTo<> derives from Subscribe<Data>
             but has no receive(const Data&)'
```

What remains is style: a project that enables `-Wsuggest-override` is warned in dynamic builds
(`'receive' overrides a member function but is not marked 'override'`); `/W4` and `-Wall -Wextra -Wpedantic` are
silent.

The alternative that was set aside keeps `receive` virtual in static builds so `override` receivers compile
untouched (`SUB0PUB_SPIKE_STATIC_VIRTUAL`). The publish path is unchanged for a `final` receiver, but it costs a
vptr per subscribed type and the retained vtables: for the station's three receivers, RAM +384 / +144 / +128 B and
text +936 / +239 / +304 B (MSVC / clang-cl / icx). The knob stays in the spike as the measurement behind this
decision.

---

## 5. Semantics that still differ

Differences in meaning between a brokered type and a wired one. Converging the spelling must not hide them.

### 5a. Delivery order

**Decided: document it, with this example.**

```cpp
Audit audit;          // constructed first
Display display;
struct Reading { int celsius; using sub0_config = sub0::config<sub0::spike::StaticTo<&display, &audit>>; };   // static build only
```

```text
brokered:  audit display        registration order, which is construction order
wired:     display audit        the order of the list
```

A program whose receivers depend on each other's side effects changes behaviour when a type is promoted. List
receivers in construction order where order matters. The wired order is at least written in one line; the
brokered one is spread over the program. The station example's header states it.

### 5b. The set of receivers is closed

**Direction: safe by default, with an opt-out; and give publishers a way to ask.**

```cpp
Display display;      // listed in StaticTo<&display>
Display spare;        // not listed
```

Release build: `display 1 spare 0`. Debug build, at `spare`'s constructor:

```text
Assertion failed: sub0pub: this subscriber is not listed in its type's StaticTo<>
```

- *Safe by default.* The check is on in debug builds.
- *Opt-out.* It is now a hook, `SUB0PUB_SPIKE_UNLISTED_RECEIVER(what)`, like the library's other violation hooks. A
  unit test that constructs one `Display` on its own under the project's static topology can make it count or log
  instead of abort, or switch the check off for that build (`SUB0PUB_SPIKE_CHECK_BOUND`).
- *Asking.* `receiverCount<T>()` is the number of receivers a publication of `T` reaches now: a constant for a
  wired type (usable in a `static_assert`), the table's count for a brokered one. `tryPublish()` returns it for
  one publication. Both are in the spike (decision 2).

*The audit.* In an audit build the same condition is a finding instead of an abort, alongside its mirror image
for a brokered type, a subscriber that never received anything:

```text
struct Status: a subscriber at 000000122AEFFA30 subscribes to it but is not in its StaticTo<> list, so it is never called
struct Reading: class Logger at 000000122AEFF970 never received it: it subscribed after the last publication
```

Not done: a form of the "not listed" check for an ordinary release build, which would need a registration step
the static path does not otherwise have. The audit is the release-build answer.

### 5c. Features that exist only on the broker

**Deferred.** Using one on a statically routed type is a compile error today, which lists what a promotion has to
resolve:

```cpp
display.disconnect();
```

```text
error C2039: 'disconnect': is not a member of 'Display'
```

The same is true of `trySubscribe()`, `isSubscribed()` and `sub0::cancel()`. For later consideration: some of
these have a sensible static meaning. `isSubscribed()` could answer "is this object in its type's list?", which
makes it a validity check of the static wiring and keeps code that asserts on it portable across both models. The
spike already computes that answer for the 5b check, and the audit reports it for every subscriber at once, which
may be the better place for a validity check than a member each caller has to remember to ask.

`Route`, `Domain` and publish reports are not covered by the spike. `Route` derives from `sub0::Subscribe`
directly, not from the spike's alias, so here it would compile for a `StaticTo` type, register with a broker that
is never published to, and send nothing. In-library, where `Subscribe<T>` itself follows the topology, it has to
be made an error, or given a meaning, like the others.

### 5d. Cancellation

**Deferred until after the first convergence API lands.** The two models stop a publication in different ways:

```cpp
// broker: needs a publish context (SUB0PUB_CANCEL or sub0::ThreadLocalContext)
void receive(const Reading& reading) noexcept override { if (reading.celsius > 90) cancel(); }

// wiring: the receiver returns bool, and the publisher calls publishCancelable
bool receive(const Reading& reading) noexcept { return reading.celsius <= 90; }
```

On a `StaticTo` type the first form does not compile, and the second is silently ignored, because the spike's
`publish()` calls `StaticWiring::publish`, not `publishCancelable` (limitation K19). Until this is taken up, a
type that needs cancellation stays on one model. A small step that could land with the first version without
prejudging the design: make a `bool`-returning `receive` on a `StaticTo` type a compile error, so it cannot be
ignored unnoticed.

### 5e. `filter()`

**Decided: an opt-in per type, in both models**, as it always should have been, and as cancellation is: a feature
the type's configuration has to ask for before it is even considered.

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
| default, brokered | `error C2555: 'Display::filter': overriding virtual function return type differs` |
| `sub0::Filter`, brokered | works: `-5` is filtered |
| `StaticTo<&display>` | `error C2338: a receiver listed in StaticTo<> declares filter(), but its message type is not configured with sub0::Filter` |
| `sub0::Filter` + `StaticTo<&display>` | works: `-5` is filtered |

The third row is new. Before it, a wired type honoured `filter()` whether or not the type had opted in, so the
static build was more permissive than the dynamic one. Now one source is accepted or rejected alike.

---

## 6. `StaticFirst`, and the audit

**Direction:** `StaticFirst` is a user convenience and stays one; how much of an application is loaded dynamically
is the user's to know, not ours. What helps more is tooling: let users write free and easy dynamic binding, then
show them, from a run, what actually happened. One recording can carry most of the checks this document has been
adding one at a time, so the audit is built as the single source of truth for them.

### `StaticFirst` as it stands

One line promotes a known receiver while the type stays open:

```cpp
struct Reading { int celsius; using sub0_config = sub0::config<sub0::spike::StaticFirst<&display>>; };
```

Against `StaticTo` with the same receivers and nobody subscribed at run time:

| | MSVC | clang-cl | icx |
|---|---|---|---|
| `one_receiver`, instructions per publication | +11 | +9 | +9 |
| `station` (two types), instructions per publication | +21 | +18 | +18 |
| `batch`, a burst of 64: instructions | 732 against 205 | 149 against 136 | 739 against 250 |
| `batch`: is the burst still vectorised? | no | yes | no |

It keeps the broker's table and registration code in the image (RAM +350 to +420 B for one type), and it is the
one mode in which a bound receiver's `receive()` stays virtual. Its documentation should say not to use it on a
burst path. The spike registers and then disconnects a bound receiver; an in-library form would decide first.

### The audit

[`audit.hpp`](include/sub0pub_spike/audit.hpp). It is a build mode, not a source change: the program is compiled
with one extra definition,

```text
-DSUB0PUB_CONFIG_HEADER="sub0pub_spike/audit_build.hpp"
```

and then keeps one ledger per message type, fed by both delivery paths (the runtime broker and a `StaticTo<>`
list). At exit it prints its findings, then each type's publishers and receivers.

**A clean program.** The station, built unchanged as an audit build, in its dynamic mode (MSVC):

```text
sub0pub audit: 0 findings
  struct Reading: 3 publications; runtime table peaked at 2 of 8
    published by class Thermometer: 3
    1. class Display at 00007FF62D7E6408: 3 deliveries
    2. class Audit at 00007FF62D7E63E0: 3 deliveries
    the receivers never changed: a candidate for StaticTo<>, in this order, if they have static storage
  struct Alarm: 1 publication; runtime table peaked at 1 of 8
    published by class Thermometer: 1
    1. class Audit at 00007FF62D7E63F0: 1 delivery
    the receivers never changed: a candidate for StaticTo<>, in this order, if they have static storage
```

That is the guided route from dynamic to static: the list to write, in delivery order, read off a run. The same
station in its static mode, where the audit names each listed receiver by the object in the list:

```text
sub0pub audit: 0 findings
  struct Reading: 3 publications
    published by class Thermometer: 3
    1. display (class Display) at 00007FF64C7561C8: 3 deliveries, wired
    2. audit (class Audit) at 00007FF64C7561C0: 3 deliveries, wired
```

**A program with mistakes.** [`examples/audit_findings`](examples/audit_findings/main.cpp) makes five, one per
message type, none of which stops it:

```cpp
thermometer.measure(95);    // 1. the Alarm is raised before any AuditLog exists
AuditLog auditLog;
thermometer.measure(20);
Logger logger;              // 2. attached after the last Reading
Console console;            // 3. waits for a Maintenance message that nothing publishes
Listener first;
Listener second;            // 4. Beacon's table holds one subscriber
Panel spare;                // 5. not in Status's StaticTo<&panel>
```

```text
sub0pub audit: 6 findings
  struct Status: a subscriber at 000000122AEFFA30 subscribes to it but is not in its StaticTo<> list, so it is never called
  struct Reading: class Logger at 000000122AEFF970 never received it: it subscribed after the last publication
  struct Alarm: 1 of 1 publications reached no receiver; 1 by class Thermometer
  struct Alarm: class AuditLog at 000000122AEFF960 never received it: it subscribed after the last publication
  struct Maintenance: never published, although 1 receiver subscribed to it
  struct Beacon: 1 subscription refused, the table of 1 was full
```

The first mistake shows from both sides: a publication nobody received, attributed to its publisher, and a
receiver that got nothing. The example's test passes when `auditFindings()` returns exactly six.

**A unit that missed its configuration** (the `SUB0PUB_CONFIGURE` limitation of decision 2), as an audit build:

```text
sub0pub audit: 2 findings
  struct Reading: 1 of 2 publications reached no receiver; 1 by struct LegacySensor
  struct Reading: published with different topologies in different translation units; one of them missed its configuration
  struct Reading: 2 publications
    published by struct Thermometer: 1
    published by struct LegacySensor: 1
    1. display (class Display) at 00007FF7193EED3C: 1 delivery, wired
```

It names the publisher at fault, which neither the debug assertion nor the in-publish check does.

**Using it in a test or in CI.** `auditFindings()` returns the count so far, for an assertion at the end of a
test. `SUB0PUB_SPIKE_AUDIT_EXIT(findings)` runs after the report at exit; defining it to exit non-zero fails any
run that has a finding (verified: the program above then exits with the hook's status).

**What it answers, and what that could retire.**

| Question | Was | In the audit |
|---|---|---|
| Did a publication reach nobody? (2) | the in-publish check and its hook | a finding, with the publisher; not fatal |
| Is a subscriber missing from its type's list? (5b) | a debug assertion at construction | a finding |
| Was a receiver ever called? (K14, 5c) | not checked for a brokered type | a finding |
| Did every unit agree on the topology? (2) | a debug assertion at the publication | a finding, with the publisher |
| Is the subscription table big enough? | `SubscribeResult` at each call site | a finding, and the peak against the capacity |
| Could this type be wired, and how? (6) | not available | the receivers in delivery order, and whether the set ever changed |
| Who publishes this type? | not available | each publishing class and its count |

The compile-time checks stay as they are; they need no run. The two debug assertions are what the audit could
replace: they stop at the first problem and say nothing about who caused it, where the audit collects everything
a run did. Whether to retire them or keep them as the cheap default is a choice to make once the audit is more
than a spike.

**Limits of the spike.**

- It reports one run. A plug-in that loads on the tenth run was not seen; a tool would merge reports.
- The brokered side stands in for the library broker's default configuration, through `Implementation<>`. A
  program with a type that has a lock, a publish context or a `Domain` does not compile as an audit build, and a
  project that already names a `SUB0PUB_CONFIG_HEADER` would have to include the audit's from it.
- It is single-threaded and remembers 16 receivers and 8 publishing classes per type.
- A brokered receiver is named from its first delivery, or when any subscriber next joins or leaves; one that is
  created last and never called is reported by address only. A subscriber of a wired type that is not in the list
  is always by address: its base class has no virtual function to ask.
- Publishers are attributed by class, not by object, and only through the converged `publish()`. A publication
  made with `sub0::publish()` directly is counted as unattributed. A class that derives from `Publish<T>` and
  never publishes is not seen at all.
- It cannot tell whether an object has static storage, so a candidate list still has to be checked for that.
- It needs RTTI. (MSVC 19.51 evaluates `typeid(*p)` at compile time, naming the base, when `p`'s class is a
  template specialisation not yet instantiated at that point; the audit requires the class's size first, which
  makes it read the object. Worth knowing for any in-library form.)

**What the fuller tool would be:** the same ledger fed by the library broker itself, for every configuration;
the report written in a form a script can merge across runs and turn into the configuration lines.

---

## 7. Header layering

**Deferred: no hard decision now.** Recorded for when it is taken up.

STYLE_GUIDE.md: "An area's entry header must not reach another area; bridges between areas get their own header."
`sub0::publish()` and `sub0::Subscribe<T>` are in the broker area, and a statically routed publish is wiring. It
does not need an include: the spike finds the topology by asking the type's configuration whether it has one, and
then calls a member of it.

```cpp
template<class Config>
    requires requires { typename Config::topology; }
struct TopologyOf<Config> { using type = typename Config::topology; };

if constexpr (Topology::cHasStatic)
    Topology::publish(data);            // a dependent call: resolved where publish() is instantiated
```

Nothing there names `StaticWiring`. The header that *defines* `StaticTo` is the one that includes wiring, and that
is a bridge header in the sense the rule intends, like the existing `sub0pub/wiring/broker_port.hpp`:

```text
sub0pub/config.hpp            options and resolution; knows nothing of topologies
sub0pub/broker/*.hpp          Subscribe / Publish / publish: follow Config::topology if it exists
sub0pub/wiring/*.hpp          StaticWiring: unchanged
sub0pub/wiring/static_to.hpp  StaticTo<&...> : the bridge; includes wiring
```

The alternatives are to let `broker/publish.hpp` include `wiring/capability.hpp` and amend the rule, or to keep
`sub0::publish` broker-only and give the converged entry points new names, which gives up the main result:
existing broker code would no longer be the converged spelling.

---

## 8. Configuring a type without the preprocessor

**Deferred: no hard decision now.** Decision 2 narrows it: with configuration next to the type as the default,
`SUB0PUB_CONFIGURE` is already confined to types that cannot be edited.

Probed on MSVC 19.51 and clang-cl 22 by a separate agent session (14 candidate spellings); I re-compiled the first
form below.

The macro is sugar for one explicit specialisation, which can be written directly and shortened by a one-line
helper base, with no change to how configuration resolves:

```cpp
namespace sub0 { template<class... Options> struct configured_with { using type = config<Options...>; }; }   // the helper

template<> struct sub0::configure<Reading> : sub0::configured_with<sub0::spike::StaticTo<&display, &audit>> {};
template<> struct sub0::configure<int>     : sub0::configured_with<sub0::Capacity<32>> {};
```

It works on both compilers, with `&display` still an incomplete `extern` object, and for `int`. Declared after the
type's first use it is a hard error, which is the behaviour to want. Like the macro it must be at global scope or
inside `namespace sub0 { }`:

```text
error C2888: 'sub0::configure<app::Reading>': symbol cannot be defined within namespace 'app'
warning: class template specialization of 'configure' not in a namespace enclosing 'sub0' is a Microsoft extension
```

The other candidate worth keeping in view is one list in a project header, found by a pack search during
resolution. It can live in any namespace and can never be declared late, and it needs only forward declarations.
It needs a change to `config_t` resolution, was run against a copy of that machinery rather than `config.hpp`
itself, and its compile-time cost is unmeasured. It is also the build-system-level mechanism that decision 2 set
aside as the default, so it would be an addition for the cannot-edit case, not a replacement.

---

## What changed in the spike while working through these

None of it changes generated code for a publication that has receivers; the recorded results were regenerated
afterwards.

| Change | Decision | Catches or provides | When |
|---|---|---|---|
| a listed receiver that derives from `Subscribe<T>` must have `receive(const T&)` | 4 | a mismatched signature silently skipped | compile time |
| a listed receiver may declare `filter()` only if the type has `sub0::Filter` | 5e | a filter honoured without the type's opt-in | compile time |
| a `StaticTo<>` list must contain a receiver of the type, unless `AllowNoReceivers` | 2 | a publication that can reach nobody | compile time |
| `ReportNoReceivers` (stand-in broker), `AllowNoReceivers`, `tryPublish()`, `receiverCount<T>()` | 2, 5b | a brokered publication that reaches nobody; the count on request | run time; free when there are receivers |
| the topology a unit publishes a type with must match every other unit's | 2 | a unit that missed a `SUB0PUB_CONFIGURE` | debug builds |
| a subscriber must be listed in its type's `StaticTo`; now an overridable hook | 5b | a receiver left out of the list | debug builds |
| the audit: one ledger per type, fed by the broker and by `StaticTo<>` | 2, 5b, 5c, 6 | unheard publications by publisher; receivers never called, not listed or refused; units that disagree; the wiring a type could be given | an audit build |
| the station example configures each type beside its definition | 2 | | |
