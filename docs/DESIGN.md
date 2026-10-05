# Sub0Pub v2 design

Sub0Pub's design intent is **correctness without cost**: wherever an application's topology and behaviour allow it,
the compiler must be able to remove dispatch, registration, storage and context machinery, and only a genuinely
dynamic boundary pays for runtime machinery. Every decision below was chosen by measurement against equal-work
hand-written code ([EVIDENCE.md](EVIDENCE.md)); the costs quoted are from that evidence.

The design research behind this document (prototypes, face-offs between competing mechanisms, dated reports and
review records) is preserved at the git tag `v2-research-archive`. It is not needed to use or maintain the library.

## Two dispatch structures

| | Static wiring (`sub0pub/wiring.hpp`) | Runtime broker (`sub0pub/broker.hpp`) |
|---|---|---|
| Receivers | plain classes with a non-virtual `receive(const T&)` | classes derived from `Subscribe<T>` (virtual `receive`) |
| Bound | at the application's composition point: `StaticWiring<&a, &b>`, `wire(a, b)` | at run time, by constructing a subscriber |
| Cost | the same final image as hand-written direct calls (static storage), or as hand-written runtime binding (`wire`) | a per-type subscription table and dispatch loop, configured per message type |
| Use for | hot paths and any topology known when the application is composed | subscribers that come and go, and features that need a registry: capacity reporting, filters, locking, sessions, transport routes |

The two meet at an explicit boundary:

- **`DynamicPort<T, N>`** is a fixed slot array bound into a static wiring as one more receiver. It costs what a
  hand-written registry with the same features costs.
- **`BrokerPort<T>`** forwards a static wiring's messages into the runtime broker, when the dynamic side needs the
  broker's policies (Snapshot dispatch, a lock, a `Domain`).
- **Transports:** `Forward<Transport>` and `StaticForward<&transport>` bind a transport endpoint into a wiring, with
  split horizon (`publishFrom`) so ingress is not echoed back out; `Route<T, Transport>` does the same for the
  runtime broker and can report transport rejection (`PublishReport`).

### Recommended hot-path pattern

1. Write receivers as plain classes with non-virtual `receive(const T&)`, and `filter()` only where it filters.
2. Bind instances where the application composes itself: `StaticWiring` for objects with static storage duration
   (the common embedded case), `wire(...)` when lifetimes are dynamic. A publisher holds the returned wiring by value.
3. Publishers, in order of cost: name the `StaticWiring` alias directly; use the CRTP mixin
   `Publisher<Derived, Out>` when the topology is not known where the publisher is written; use `Sink<T>` across a
   library or ABI boundary (one indirect call, the price of type erasure itself).
4. A receiver that stops the rest of a publication returns `bool` (`false` stops), and the publisher calls
   `publishCancelable`. No shared or thread-local state is involved.
5. Put genuinely dynamic subscribers behind a `DynamicPort`, or a `BrokerPort` when they need broker policy.

The [examples](../examples/README.md) show each of these as a standalone program.

## Per-type configuration of the runtime broker

Every `Subscribe<T>`, `Publish<T>` and `publish()` for a type `T`, in every translation unit, must agree on how `T`
is brokered, so the configuration is **a property of the type**. It is resolved in this order:

1. **Per type, exactly one of** (configuring a type in two places is a compile error):
   - a member alias: `struct Imu { ...; using sub0_config = sub0::config<sub0::Capacity<2>>; };`
   - an ADL declaration next to the type: `sub0::config<...> sub0_config(gps::Fix*);`
   - a traits specialisation for types you cannot modify: `SUB0PUB_CONFIGURE(int, sub0::Capacity<32>);`
   - a `Tagged<Payload, Tag>` payload, whose tag carries the member alias
2. **The project default:** a header named by `SUB0PUB_CONFIG_HEADER`, set by the build system so every
   translation unit agrees, which defines `SUB0PUB_DEFAULT_CONFIG`.
3. **Builtin:** the `SUB0PUB_*` macros.

`sub0::config<Opts...>` means "the project default with these options applied". Consistent visibility of a type's
configuration is a build contract: a type that resolves differently in two translation units is an ODR violation.
The member alias, ADL declaration and `Tagged` satisfy it by construction; a debug-build check reports mismatches it
observes at run time.

| Axis | Options | Default | What it buys, and its price |
|---|---|---|---|
| Dispatch | `Direct`, `DirectChecked`, `Snapshot` | `Direct` (checked in debug builds) | `Snapshot` allows subscribing, unsubscribing or destroying a subscriber from inside its own dispatch; it copies the table per publish and needs a publish context |
| Context | `NoContext`, `ThreadLocalContext`, `StaticContext` | none | a publish frame for `cancel()`, routes and publish reports; thread-local storage, or a static frame for single-threaded targets without TLS |
| Filter | `Filter`, `NoFilter` | `NoFilter` | `Subscribe<T>::filter()`: one virtual call per subscriber per publish |
| Lock | `LockWith<L>` (any `lock()`/`unlock()` type) | none | concurrent publishers and cross-thread teardown; requires `Snapshot` and `ThreadLocalContext` |
| Storage | global, `Scoped` | global | `Domain<T>` instances: isolated sessions of one type, with `close()` |
| Capacity | `Capacity<N>` | `SUB0PUB_MAX_SUBSCRIPTIONS` (8) | RAM only: the table is sized per type |
| Implementation | `Implementation<Broker>` | the library broker | a custom broker for a special case (for example one subscriber, one pointer of RAM) |

Every option that costs something is opt-in, and using a feature without opting in is caught: `cancel()`, routes,
publish reports and `filter()` do not compile without their option; invalid combinations (a lock without Snapshot,
Snapshot without a context, a lock with `StaticContext`, `Domain<T>` for a non-scoped type) are compile errors; table
changes during a Direct dispatch are reported by the debug-build check. [MIGRATION.md](../MIGRATION.md) lists the
opt-in for each v1 behaviour.

## Decisions

| # | Decision | Chosen | Rejected, with the measured reason |
|---|---|---|---|
| D1 | Where a type's broker policy lives | On the type (member alias, ADL declaration, `SUB0PUB_CONFIGURE`, `Tagged`), else the project header, else the macros | a broker chosen at each use site (sites can silently disagree); a central registry header of every type (dependency inversion) |
| D2 | What the default costs | The cheapest correct dispatch: direct iteration, no publish context, no `filter()`, no lock | snapshot, `cancel()` and `filter()` always on: 77 / 287 instructions per publish against 38 / 101 for 1 / 8 subscribers |
| D3 | The hot-path structure | Static wiring at the composition point | policy switches inside the virtual registry, which keep its dispatch model; routing static receivers through the registry: +78 to +86 publish instructions |
| D4 | The static-to-dynamic boundary | `DynamicPort`, or `BrokerPort` for broker policy | a registry in front of the static wiring: publish +37 (GCC) / +10 (Clang) over the hand-written equivalent |
| D5 | Cancellation on the static path | `bool` result with `publishCancelable` | a thread-local `cancel()` flag: GCC +3 instructions; Cortex-M33 +14 path instructions, +257 B RAM and a TLS dependency |
| D6 | Publisher spelling | the `StaticWiring` alias, the CRTP `Publisher` mixin, or `Sink<T>` | a CTAD factory (Clang +4 instructions, +24 B); C++23 deducing this (GCC 13 rejects it, and it costs the same as the mixin) |
| D7 | Lifetime | `Subscribe<T>` and `Publish<T>` have protected, non-virtual destructors; locked types register with `trySubscribe()` after construction; disconnect during a dispatch is safe under Snapshot; `Domain::close()` detaches, rejects and quiesces | a virtual destructor (a vptr per object and an `operator delete` link dependency on small targets); registration in the base constructor for concurrent types (another thread could dispatch into a half-built object) |
| D8 | Teardown under concurrency | a sequentially consistent handshake: `disconnect()` waits only for a callback running on another thread | hazard pointers and epochs: cheaper only because they drop self-disconnect, nested-publish and thread-count safety, and they drop publications past their bounds in release builds |
| D9 | Language standard | C++23 is the contract; use concepts/requires where they simplify constraints | retaining C++17 compatibility scaffolding; adopting poorly supported features without toolchain and equal-work evidence (see "Language baseline" below) |

## Contracts

- **Subscriber lifetime.** After `disconnect()` returns, `receive()` is not called again, on any thread. When other
  threads may publish, call `disconnect()` from the most-derived destructor, before derived state is destroyed.
- **Registration.** Unlocked subscribers register in their constructor. Locked (concurrent) types do not: call
  `trySubscribe()` at the end of the most-derived constructor. A full table is reported
  (`SubscribeResult::CapacityExceeded`), never overrun.
- **Domains** must outlive the handles bound to them.
- **Wirings add no synchronisation.** Concurrent publishers on one wiring need stable bindings and thread-safe
  receivers. `DynamicPort` is single-threaded: adding, removing and publishing must not overlap.
- **Transports.** `Forward` ignores send results; `Route` records them. Neither means remote delivery. Split horizon
  prevents an immediate echo to the link a message arrived from, not arbitrary network cycles.

## Known limitations

Each limitation has a measured price or a documented usage rule. Identifiers are stable, so tests and comments can
refer to them.

| # | Limitation | Price | Route to removing it |
|---|---|---|---|
| K1 | Teardown safety on create + destroy: a registration flag and, with a publish context, a check for dispatches in progress | create + destroy 47 instructions in the default (v1.0: 48; GCC, compare-v1-v2 report) | skip the check when no dispatch of the table is active on this thread |
| K2 | Types with a publish context carry a dispatch frame (origin, report, snapshot) even without routes | Snapshot, 1 subscriber: +2; Direct, 0 subscribers: +7 | a minimal frame for types without routes |
| K3 | Locked types pay a handshake per subscriber per publish, and a second lock acquisition | `std::mutex`, 1 / 8 subscribers: 260 / 554 instructions per publish | per-subscriber reference counts or epochs, if they can pass the same lifetime tests |
| K4 | Concurrent `disconnect()` blocks for at most one callback on another thread; two receivers disconnecting each other at once from different threads deadlock | a usage rule | a non-blocking `disconnectLater()` for use inside receivers |
| K5 | Locked types need an explicit `trySubscribe()` after construction | easy to forget | a CRTP helper that activates after construction |
| K6 | `Domain` lifetime is only debug-checked; configuration consistency across translation units is a build contract with a best-effort check | undefined behaviour if violated in a release build | link-time detection |
| K8 | `Implementation<>` brokers support global storage only | `Domain` needs the library broker | make the table type part of the broker concept |
| K9 | `cancel()`, re-entrancy checks and teardown walk this thread's dispatch frames | O(nesting depth), usually 1 | per-table frame chains if deep nesting appears |
| K10 | The cross-thread teardown test is probabilistic | a removed wait is caught in about 4 of 5 runs | a deterministic interleaving harness |
| K13 | C++23 feature coverage varies across host and embedded compilers | selecting C++23 mode does not provide every language/library feature | validate each newly used feature in the supported matrix; keep CRTP until explicit-object-parameter support and cost are established |
| K14 | Capability routing is silent on a signature mismatch: a receiver whose `receive` does not accept the message is skipped without a diagnostic | a missed delivery, found only by tests | state it where it is bound: `static_assert(sub0::handles_v<R, T>)` |
| K15 | Split horizon is decided at compile time when the origin's type is bound once; the origin must then be that endpoint | a debug assertion | `publishFrom<Endpoint>(msg)` identifies the origin by type |
| K16 | A lock requires `ThreadLocalContext`, so concurrent configurations need TLS | `__aeabi_read_tp` on Cortex-M | a context keyed by the RTOS thread |
| K17 | Nothing is shared between message types: each instantiates its own table and dispatch loop | Cortex-M33, per further type: +220 B text and +49 B RAM (default), +386 B and +53 B (Snapshot, context, filter) | a type-erased dispatch core shared by types with the same configuration |
| K18 | `wire(...)`: split horizon between two endpoints of the same transport type is an address compare | publish +8 (GCC) / +10 (Clang) | give each link its own adapter type and use `publishFrom<Link>(msg)` |
| K19 | `false` from a receiver stops only `publishCancelable`; plain `publish()` ignores it, and `Sink<T>` has no cancelable publish | a missed stop | a diagnostic when `publish()` reaches a bool-returning receiver |
| K20 | `StaticWiring` needs static storage: no locals; array-element NTTP support varies by compiler; a bound array binds nothing, and fan-out is unrolled | 32 receivers: +668 B Cortex-M33 text against a hand-written loop | `wire(...)` for dynamic lifetimes; an array binding that delivers by loop |
| K21 | `DynamicPort` has no snapshot: a receiver removed during delivery makes the next one miss that publication; `add()` at capacity drops silently (`tryAdd()` reports) | x86 image +30 to +68 B | `BrokerPort` with Snapshot where removal during delivery is needed |
| K22 | A nested publication on the same static wiring compiles as recursion through the fold | GCC x86 +536 B text; Clang +4 instructions; Cortex-M33 none | publish nested messages through a separate wiring |
| K23 | Clang does not propagate `Wiring` bindings held inside an aggregate as it does a struct of pointers | publish +3 to +5, RAM +24 B (Clang only) | open |
| K24 | Cancellation combined with `filter()` is not byte-identical to hand-written code | Cortex-M33 +4 path instructions, +12 B; GCC +3 | open (small) |
| K25 | `Publish<T>` has a protected destructor, so a publisher is always a derived class | one line per publisher type | a library-provided final handle type |
| K26 | Under `Direct` dispatch, a table change during that table's own dispatch is detected only by the debug-build check | a release build may skip a subscriber or call one added during it | opt in with `Snapshot` for types that change their table from their own callbacks |

Not yet measured: throughput under lock contention and teardown latency, embedded stack use (Snapshot copies the
table to the stack, so size `Capacity` to the real bound), and cross-module (DLL / shared library) use, which is not
supported yet ([examples/cross_module](../examples/cross_module/README.md)).

## Language baseline

`Sub0Pub::Sub0Pub` exports `cxx_std_23`, and the configuration header rejects C++17 and C++20 builds that include the
headers directly. A C++23 mode does not guarantee every C++23 feature (K13), so a feature is adopted only for a concrete
simplification, with compiler coverage and unchanged semantics:

| Area | Decision | Reason |
|---|---|---|
| Static-wiring capability detection | requires-expressions | states each capability directly; explicit-`bool` filters, exact-`bool` cancellation and skipped non-matching receivers are unchanged and tested |
| `Sink` copy exclusion | a requires-clause on the binding constructor | copying a `Sink` copies it and never wraps it |
| Broker configuration detection | kept as is | carries MSVC ADL workarounds; replacing it needs cross-compiler evidence |
| Publisher CRTP mixin | kept | explicit object parameters need toolchain coverage and equal-work evidence first (D6) |
| Result enums, `const T&` payloads | kept | `std::expected` suits a future admission boundary, not synchronous publication; queue ownership belongs in an adapter ([INTEGRATION.md](INTEGRATION.md)) |
| Synchronisation and storage | unchanged | their lifetime and cost contracts do not depend on the language mode |

## Compilation cost

The library is header-only, so parsing and instantiation repeat in every consumer translation unit. Build time is
measured separately from runtime cost ([COMPILE_TIME.md](COMPILE_TIME.md)):

- Aggregate arity detection selects the recursive type before requesting its value, so only one branch of its binary
  search is instantiated at each step (32 members at most, unchanged).
- The narrow broker headers do not include `<algorithm>`: snapshots copy pointer arrays with `memcpy`, and
  `Domain::close()` records the detached subscribers in its clearing pass, only where the single-threaded path uses them.
- `<thread>` stays in the broker headers: any message type may opt in to a lock, whatever the global defaults say.
- No precompiled headers, modules, unity builds or a type-erased broker core: each trades portability, integration or
  runtime cost, and needs its own equal-work evidence.

The IPC buffer registry stays a fixed-capacity sorted array with binary lookup. `trySet()` reports a full registry
before moving entries or touching padding, and replacement succeeds when full; this is registration-path work only.
