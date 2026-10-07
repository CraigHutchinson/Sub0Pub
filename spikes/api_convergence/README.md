# Static/dynamic API convergence spikes

Sub0Pub has two dispatch structures (docs/DESIGN.md): the runtime broker and static wiring. Today they are also two
*programming models*: choosing one decides how every receiver and publisher is written, so moving a message type from
the broker to direct calls means rewriting its participants. These spikes look for an API in which that choice is a
configuration, not a rewrite, without giving up what either path costs today.

They are review spikes, not a proposed production change. The library is untouched: each candidate is a thin layer
in [`include/sub0pub_spike/`](include/sub0pub_spike/) over the public API, so a layer can only add cost to what an
in-library form would have. How the candidates are measured is in [WORKFLOW.md](WORKFLOW.md); the recorded
measurements are in [`results/`](results/).

## Result

One candidate, the **typed route**, meets the goal on everything measured here:

- A message type's delivery becomes one more axis of its per-type configuration, written next to the type.
  Participants are written once, in the runtime broker's spelling. Switching a type to direct calls adds 4 to 8
  lines beside the message types and changes no line of any publisher or receiver.
- Its dynamic build is the current API by construction (the same types), and measures identical to it on every
  metric, on three compilers.
- Its static build equals hand-written direct calls on instructions per publication, publish path, RAM and image in
  all four cases on clang and icx. On MSVC it equals today's static API except for a three-instruction
  construction cost caused by that ABI's empty-base layout (removed by `__declspec(empty_bases)`). A burst of
  publications vectorises exactly as the hand-written loop does.

The other two candidates are useful as what they already are: explicit wiring remains the lower-level lever, and a
type-erased port remains the tool for a library boundary. Neither converges the two models.

**GCC, callgrind and Cortex-M33 were not available on the machine these were measured on** (MSVC 19.51, clang 22,
Intel icx 2026.1, all x64 Windows). That evidence is still owed before the typed route moves into `include/`.

The design decisions that measurement could not settle are worked through, each with compiled examples, in
[DECISIONS.md](DECISIONS.md). Six are decided or have a direction as of 2026-10-07; the rest are deliberately
deferred until a first in-library version exists.

## The problem

| | Runtime broker | Static wiring |
|---|---|---|
| Receiver | derives from `Subscribe<T>`, overrides a virtual `receive` | a plain class with a non-virtual `receive` |
| Publisher | derives from `Publish<T>`, calls `sub0::publish(*this, msg)` | a template on its output, calls `out.publish(msg)` |
| Composition | none: constructing a participant registers it | the receivers are listed: `StaticWiring<&a, &b>`, `wire(a, b)` |
| Set of receivers | open: they come and go at run time | closed: fixed where the application is composed |

The last row is a real difference in meaning and no API should hide it. The first three are differences in spelling,
and they are what make "start dynamic, optimise the known types later" expensive: in the `status_quo` example, the
switch touches 29 of the station's lines, 20 of them inside the publisher and receiver classes.

## Candidates

Each is one weather station under [`examples/`](examples/): a thermometer publishes a `Reading` per measurement and
an `Alarm` when it overheats; a display receives readings; an audit log receives both. Sources outside `mode/` are
shared by every build of that candidate.

### Status quo: the two current APIs (the control)

[`examples/status_quo`](examples/status_quo/). The participants exist twice, once per model.

### Bus parameter: write publishers against an output type

[`examples/bus_parameter`](examples/bus_parameter/). Receivers are plain classes; the publisher is
`Thermometer<Bus>`. Two small additions make the broker one more bus: `BrokerBus` presents it as a wiring, and
`Subscription<Owner, Ts...>` joins a plain receiver to it.

```cpp
struct Station                                              // dynamic
{
    using Bus = sub0::spike::BrokerBus;
    Display display;
    Audit audit;
    sub0::spike::Subscription<Display, Reading> displaySubscription{display};
    sub0::spike::Subscription<Audit, Reading, Alarm> auditSubscription{audit};
    Thermometer<Bus> thermometer{Bus{}};
};
```

For: explicit and local; several independent buses; nothing global; a test can pass its own bus. Its static build
*is* today's static API.
Against: every publisher, and everything that holds one, is a template. A dynamic receiver needs a second object
with a lifetime-ordering contract, and that adapter costs one instruction per delivery to a stateful receiver and
roughly 190 to 310 bytes of RAM per receiver over deriving from `Subscribe<T>`: the dynamic path gets slightly
worse. For existing broker code it is a third model to migrate to, not a convergence.

### Port member: type-erased outputs

[`examples/port_member`](examples/port_member/). Receivers are plain classes; the publisher is an ordinary class
holding a `Sink<T>` per message type, bound where the station is composed to the broker or to a wiring.

For: publishers stay non-template classes that can live in a `.cpp` or behind a library boundary; the smallest
composition change after the typed route.
Against: a `Sink` is an indirect call the compiler cannot see through. The static build cannot collapse (+8 to +26
instructions per publication over direct calls, which is what a hand-written context pointer and function pointer
costs), and the dynamic build is 8 to 27 instructions per publication slower than today's. This is the price
of type erasure, already documented for `Sink`; it rules the candidate out as the hot path.

### Typed route: delivery as per-type configuration

[`examples/typed_route`](examples/typed_route/), layer in
[`topology.hpp`](include/sub0pub_spike/topology.hpp). The participants are today's broker code:

```cpp
class Display final : public sub0::spike::Subscribe<Reading>
{
public:
    void receive(const Reading& reading) noexcept { ... }   // no `override`
};

class Thermometer final : public sub0::spike::Publish<Reading>, public sub0::spike::Publish<Alarm>
{
public:
    void measure(int celsius) noexcept;                      // defined in thermometer.cpp
};

void Thermometer::measure(int celsius) noexcept
{
    sub0::spike::publish(*this, Reading{celsius});
    if (celsius >= cOverheatCelsius)
        sub0::spike::publish(*this, Alarm{celsius});
}
```

With nothing else, every type is brokered. A type says otherwise next to its own definition:

```cpp
class Display;
class Audit;
extern Display display;
extern Audit audit;

struct Reading
{
    int celsius;
    using sub0_config = sub0::config<sub0::spike::StaticTo<&display, &audit>>;   // direct calls, in this order
};
```

`StaticTo` is a configuration option like `Capacity` or `Snapshot`, so it resolves through the existing chain. The
member alias and the declaration found by argument-dependent lookup are the default spellings, because they make
the configuration part of the type; `SUB0PUB_CONFIGURE` is for a type that cannot be edited
([decision 2](DECISIONS.md#2-where-the-configuration-lives-and-publishing-to-nobody)). `Subscribe<T>`, `Publish<T>`
and `publish()` follow the topology:

| Topology of `T` | `Subscribe<T>` is | `publish(from, T{})` is |
|---|---|---|
| none (default) | `sub0::Subscribe<T>` itself | `sub0::publish` |
| `StaticTo<&a, &b>` | an empty class: no vtable, no registration | `StaticWiring<&a, &b>::publish` |
| `StaticFirst<&a>` | `sub0::Subscribe<T>`, not registered for the bound `a` | the direct calls, then `sub0::publish` |

The station builds six ways from the same sources; only `mode/<mode>/station_types.hpp` differs:

| Build | Configuration |
|---|---|
| `dynamic` | none: both types brokered |
| `static` | a member alias on each type |
| `hot_path` | an ADL declaration beside `Reading` only; `Alarm` stays brokered |
| `bridged` | `StaticFirst`: the display called directly, the audit log still subscribing at run time |
| `foreign` | `SUB0PUB_CONFIGURE`, for message types in a header that stands for someone else's |
| `report` | brokered, with `RecordWiring`: at exit it prints the wiring it observed, as a candidate `StaticTo` list |

For: the client diff and the measurements below. Publishers remain ordinary classes. The granularity is the type,
which is where "known hot" is decided. Using a dynamic-only feature (`disconnect()`, `trySubscribe()`, `cancel()`)
on a statically routed type is a compile error, so the compiler lists what a promotion has to resolve.
Against: one topology per message type for the whole program; a message type that names its receivers depends on
their declarations; publishers of a static type must see the receivers' definitions; and `receive()` is declared
without `override`.

A promotion cannot fail silently. At compile time: a listed receiver that subscribes to the type must be able to
receive it; a `filter()` needs the type's `sub0::Filter`; a list that nobody in it can receive from is rejected.
In debug builds: a subscriber left out of its type's list, and a translation unit that publishes a type with a
different topology, are reported. And a publication that reaches no receiver at all is a failure by default, for
brokered types too, at no cost to a publication that has receivers
([decision 2](DECISIONS.md#2-where-the-configuration-lives-and-publishing-to-nobody)).

## Measurements

MSVC 19.51 `/O2`, clang-cl 22.1 `/O2`, Intel icx-cl 2026.1 `/O2`; x64 Windows. Instruction counts are exact
executed-instruction counts (single-stepped), per publication, and include the driver's loop and call, the same for
every variant: compare a variant with its reference, within a compiler.

### Client cost of switching mode

[`results/client-diff-2026-10.md`](results/client-diff-2026-10.md), lines of code:

| Candidate | Switch | Removed | Added | Files | Lines edited in publisher / receiver classes |
|---|---|---:|---:|---|---:|
| status quo | dynamic → static | 12 | 17 | `compose.hpp`, `participants.hpp` | 20 |
| bus parameter | dynamic → static | 7 | 5 | `compose.hpp` | 0 |
| bus parameter | dynamic → wired | 6 | 3 | `compose.hpp` | 0 |
| port member | dynamic → static | 5 | 1 | `compose.hpp` | 0 |
| typed route | dynamic → static | 0 | 8 | `station_types.hpp` | 0 |
| typed route | dynamic → hot path | 0 | 7 | `station_types.hpp` | 0 |
| typed route | dynamic → bridged | 0 | 4 | `station_types.hpp` | 0 |
| typed route | dynamic → report | 1 | 3 | `station_types.hpp` | 0 |

The typed route's added lines are the receivers' forward declarations, one configuration line per type, and the
includes that give publishers the receivers' definitions. (Its `foreign` build shows 9 removed and 9 added only
because it takes the message types from another header instead of defining them.)

Bus parameter and port member reach zero participant edits only because their participants were written in the
static style from the start; code written for today's broker has to be rewritten once to get there. The typed
route's participants are today's broker code.

### A/B shootout

[`results/shootout-2026-10.md`](results/shootout-2026-10.md). Case `station`: two message types, three receivers,
four deliveries per publication. Instructions per publication, observable work (MSVC / clang-cl / icx):

| Variant | Judged against | Instructions | Delta | Other criteria |
|---|---|---|---|---|
| hand-written direct calls | reference | 21 / 33 / 33 | | |
| today's static (`StaticWiring`) | hand-written | 21 / 33 / 33 | 0 / 0 / 0 | all equal |
| **typed route, static** | hand-written | 21 / 33 / 33 | 0 / 0 / 0 | all equal; MSVC: setup +3 instructions, text +16 B ¹ |
| typed route, static, `override` kept ² | hand-written | 21 / 33 / 33 | 0 / 0 / 0 | RAM +384 / +144 / +128 B, text +936 / +239 / +304 B |
| bus parameter, static | hand-written | 21 / 33 / 33 | 0 / 0 / 0 | all equal |
| port member, static | hand-written, type-erased | 47 / 58 / 59 | +1 / 0 / +5 | +26 / +25 / +26 over direct calls |
| today's dynamic (`Subscribe` / `Publish`) | hand-written | 83 / 103 / 103 | +62 / +70 / +70 | the price of runtime subscription |
| **typed route, dynamic** | today's dynamic | 83 / 103 / 103 | 0 / 0 / 0 | all equal: the same image |
| bus parameter, dynamic | today's dynamic | 84 / 104 / 105 | +1 / +1 / +2 | RAM +768 / +864 / +928 B |
| port member, dynamic | today's dynamic | 110 / 123 / 124 | +27 / +20 / +21 | RAM +800 / +880 / +960 B |
| typed route, bridged, no runtime subscriber | hand-written | 42 / 51 / 51 | +21 / +18 / +18 | two empty broker tables walked per publication |

¹ The MSVC ABI gives a class with two empty bases two bytes and zeroes them when it is value-initialised. Marking
the two-base participants `__declspec(empty_bases)` (variant `route_static_empty_bases`) removes it: equal on every
criterion. Nothing on the publish path is affected.
² The subscriber base keeps a virtual `receive()` so existing `override` receivers compile unchanged
(`SUB0PUB_SPIKE_STATIC_VIRTUAL`). The receivers are `final`, so every call is still direct: the cost is a vptr per
subscribed type and the retained vtables, not instructions.

Case `one_receiver` shows the same pattern (12 / 17 / 17 for hand-written, today's static and the typed route's
static build; 34 / 40 / 40 for today's dynamic and the typed route's dynamic build).

Case `cross_file` puts the receivers' `receive()` bodies in another translation unit and names the receiver objects, with external linkage, in
a header both units include, as a shared types header does. Without link-time optimisation the typed route's
static build equals hand-written code on every criterion on all three compilers (39 / 53 / 53 instructions). With
MSVC `/LTCG` it equals today's static API (19 instructions; both are 8 B of RAM above hand-written code, the
difference docs/EVIDENCE.md already records). Its dynamic build equals today's in all four builds. Routing the type
through an out-of-line function instead, so that publishers need not see the receivers at all, costs +7 / +4 / +4
instructions per publication without link-time optimisation and nothing with it
([decision 3](DECISIONS.md#3-what-a-publisher-has-to-see)).

Reporting a brokered publication that reaches no receiver (the default of
[decision 2](DECISIONS.md#2-where-the-configuration-lives-and-publishing-to-nobody)) costs a publication that has
receivers nothing: a stand-in broker with and without the check executes 33 / 41 / 41 instructions on
`one_receiver` and 80 / 107 / 107 on `station` either way (the library broker: 34 / 40 / 40 and 83 / 103 / 103),
for 16 to 132 bytes of image.

Wall-clock time is in the report as a supplement and agrees in direction: on `station` with MSVC the static builds
run at hand-written speed, about 2.7 ns per publication, against about 7.8 ns for both dynamic builds. The timing
columns vary by tens of percent between runs (the recorded one shared the machine with other builds), which is why
instruction counts are the comparison of record.

### Vectorisation and aliasing

[`results/vectorize-batch-2026-10.md`](results/vectorize-batch-2026-10.md). Case `batch`: a burst of 64 samples
into a receiver that accumulates `value * gain`.

| Variant | clang (default / AVX2) | MSVC | icx | Instructions per burst |
|---|---|---|---|---|
| hand-written direct call | vectorised, width 4×2 / 8×4 | not vectorised (reason 1102) | vectorised | 201 / 136 / 250 |
| today's static | the same | the same | the same | 205 / 136 / 250 ³ |
| **typed route, static** | the same; alias-blocked loads 0 (hand-written: 1) | the same | the same | 205 / 136 / 250 ³ |
| hand-written, runtime-bound address | not vectorised: cannot identify array bounds; alias-blocked loads 7 | not vectorised (reason 1200) | vectorised | 417 / 373 / 248 |
| today's `wire(...)` | the same as hand-written runtime-bound | the same | the same | 417 / 373 / 248 |
| today's dynamic, and the typed route's dynamic build | not vectorised: the loop body is opaque | not vectorised (reason 1200) | not vectorised | 1369 / 1573 / 1571 |
| typed route, bridged | vectorised, width 4×2 / 8×4; alias-blocked loads 22 | not vectorised (reason 1200) | not vectorised | 732 / 149 / 739 |

Instructions are MSVC / clang-cl / icx.

³ MSVC leaves the publisher's burst function out of line in both (one call, +4 instructions); the typed route
equals today's static API there.

Two things follow. A statically routed burst is the caller's own loop again and optimises as one. And it is static
*storage*, not just static types, that buys this: when the receiver's address is a run-time value (`wire`, or
hand-written code that stores a pointer), clang and MSVC cannot prove the receiver's state does not alias the data
being published, so the running total goes through memory on every sample and the loop stays scalar: on MSVC the
runtime-bound burst takes 60 to 80 ns against under 9. That is a reason for the typed route to bind addresses as template arguments. It is
also a reason not to use `StaticFirst` on a burst path: of the three compilers only clang still vectorises the loop
there.

### Profile

[`results/vtune-station-msvc-2026-10.md`](results/vtune-station-msvc-2026-10.md): VTune hotspots, user-mode
sampling, MSVC build of `station`, about three seconds of publications per variant.

- Hand-written code, today's static API and the typed route's static build have no sampled time in any `sub0`
  function; all of it is in the receivers' own work, at 2.8 to 2.9 ns per publication.
- Today's dynamic API and the typed route's dynamic build have the same profile: 6% and 5% in
  `BrokerImpl::publish`, the rest in the receivers, at 8.3 and 7.8 ns per publication.
- The bus parameter's dynamic build shows 30% in `sub0` functions: its `Subscription` adapters, into which the
  receivers' bodies are inlined.

Hardware-event sampling, which would attribute indirect-branch mispredictions, needs an elevated prompt and was not
run.

## Recommendation

1. **Take the typed route forward as the converged API**: `StaticTo` (and `StaticFirst`, as a convenience) as
   per-type configuration options written next to the type, with `Subscribe<T>`, `Publish<T>`, `SubscribeAll` and
   `publish()` following the type's topology. It is the only candidate where existing broker code is already the
   converged spelling, and the only one whose dynamic build cannot regress, because it is the same code.
2. **Keep explicit wiring as the lower-level lever**, unchanged: `StaticWiring`, `wire`, `Publisher`, `Sink`. It
   covers what a per-type route cannot: several independent wirings of one type, receivers with dynamic lifetimes,
   a bus injected for a test.
3. **Do not pursue `Subscription` or member-bound subscriptions** as the dynamic receiver style. They exist to let a
   plain class subscribe; the typed route makes the `Subscribe<T>` base free when the type is static, which removes
   the reason, and the adapter measures worse than the base.
4. `BrokerBus` (the broker as a wiring for any type) is a small, separable convenience for the explicit-wiring API;
   decide it on its own merits.

## Decisions

[DECISIONS.md](DECISIONS.md) works through each with compiled examples, the options and their costs. Where they
stand after the owner's review of 2026-10-07:

| # | Decision | Status |
|---|---|---|
| [1](DECISIONS.md#1-one-topology-per-message-type) | One topology per message type | **Decided:** the type decides; `Tagged` where it must vary |
| [2](DECISIONS.md#2-where-the-configuration-lives-and-publishing-to-nobody) | Where the configuration lives | **Decided:** next to the type; `SUB0PUB_CONFIGURE` only for types that cannot be edited; publishing to nobody is a failure by default |
| [3](DECISIONS.md#3-what-a-publisher-has-to-see) | What a publisher has to see | Deferred to a production sample; the out-of-line route is a pattern users may apply (+4 to +7 instructions without LTO) |
| [4](DECISIONS.md#4-override) | `receive()` without `override` | **Decided:** no `override` |
| [5a](DECISIONS.md#5a-delivery-order) | Delivery order | **Decided:** documented |
| [5b](DECISIONS.md#5b-the-set-of-receivers-is-closed) | A receiver left out of the list | **Direction set:** safe by default with an opt-out; `receiverCount()` and `tryPublish()` for publishers |
| [5c](DECISIONS.md#5c-features-that-exist-only-on-the-broker) | Broker-only members on a wired type | Deferred |
| [5d](DECISIONS.md#5d-cancellation) | Cancellation | Deferred until after the first convergence API lands |
| [5e](DECISIONS.md#5e-filter) | `filter()` | **Decided:** an opt-in per type in both models |
| [6](DECISIONS.md#6-staticfirst-and-a-guided-route-from-dynamic-to-static) | `StaticFirst`; migrating from dynamic | **Direction set:** a convenience; a wiring report guides the move (proof of concept here) |
| [7](DECISIONS.md#7-header-layering) | Header layering | Deferred |
| [8](DECISIONS.md#8-configuring-a-type-without-the-preprocessor) | Configuring without the preprocessor | Deferred |

One point in decision 2 is waiting for confirmation: what a reported empty publication does in a release build by
default (abort, as the library's other violation hooks do, or only count).

**Evidence still to collect:** the shootout on GCC with callgrind and on Cortex-M33 (`-Os`, image only), which is
where the existing budgets live; compile-time A/B once the code is in `include/`; and a hardware-event VTune run,
which needs elevation.

An in-library change would touch the public API (`Subscribe`, `Publish`, `SubscribeAll`, `publish()`, the
configuration options, and the broker's behaviour on an empty publication), so it owes a `MIGRATION.md` entry, a
decision row and limitations in `docs/DESIGN.md`, tests, and collapse cases with recorded budgets.

## Layout

| Path | Contents |
|---|---|
| `DECISIONS.md` | the design decisions, each with compiled examples, and where each stands |
| `include/sub0pub_spike/` | `topology.hpp` (typed route), `no_receivers.hpp` (reporting an empty publication), `wiring_report.hpp` (the observed wiring at exit); `broker_bus.hpp` and `subscription.hpp` (the other candidates) |
| `examples/<candidate>/` | one station per candidate; `mode/<mode>/` is all that differs between its builds |
| `examples/earlier/` | the first round of spikes, kept for reference (below) |
| `cases/<case>/` | shootout variants, in the collapse-evidence harness contract |
| `harness/driver.cpp` | the collapse driver plus an instruction counter and a benchmark mode |
| `tools/` | `shootout.py`, `client_diff.py`, `vectorize_report.py`, `vtune_ab.py` |
| `results/` | recorded reports, October 2026 |

```sh
cmake -S spikes/api_convergence -B spikes/api_convergence/build
cmake --build spikes/api_convergence/build --config Release
ctest --test-dir spikes/api_convergence/build -C Release
python spikes/api_convergence/tools/shootout.py > report.md
```

## The earlier spikes

The first round, in [`examples/earlier/`](examples/earlier/), still builds and runs under ctest:

| Spike | What it tried | Outcome |
|---|---|---|
| `member_subscription` | a member-held subscription bound to a member function, by run-time pointer or as a template argument | superseded by `Subscription`, which needs one owner reference for all types and finds `receive()` by capability; then set aside with it (recommendation 3) |
| `member_wiring` | a `Wiring<...>` owned by the object that owns its receivers, spelled explicitly or deduced with `decltype(wire(...))` | carried into the bus-parameter station's `wired` mode; member order is the lifetime contract |
| `shared_flow` | one flow over both APIs, selected by the preprocessor | became the `status_quo` control |

Its second question, whether to remove the broker's virtual call by storing an object pointer and a typed callback,
is a change to the dynamic path's own cost and is still open; nothing here depends on it.
