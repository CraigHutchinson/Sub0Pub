# Sub0Pub v2 design

Sub0Pub's design intent is **correctness without cost**: wherever an application's topology and behaviour allow it,
the compiler must be able to remove dispatch, subscription, storage and context machinery, and only a genuinely
dynamic boundary pays for runtime machinery. Every decision below was chosen by measurement against equal-work
hand-written code ([EVIDENCE.md](EVIDENCE.md)); the costs quoted are from that evidence.

The design research behind this document (prototypes, face-offs between competing mechanisms, dated reports and
review records) is preserved at the git tag `v2-research-archive`. It is not needed to use or maintain the library.

## One API, and the message type decides

An application is written in one vocabulary: a receiver derives from `Subscribe<T>`, a publisher from `Publish<T>`,
and a publication is `sub0::publish(*this, msg)`. How a type `T` is delivered is a property of `T`, stated beside its
definition (D10), so every publisher and receiver of it agrees, and none of them is rewritten when the answer changes.

| Step | The real-world case | Say on the type | What a publication costs |
|---|---|---|---|
| 1 | The default: receivers subscribe and unsubscribe with their lifetimes, or the application is still taking shape | nothing | the runtime broker: a subscription table per type and a dispatch loop, configured per type |
| 2 | The type's receivers are a closed set of objects with static storage: the common embedded case, and any hot path | `StaticTo<&a, &b>` | the final image of hand-written direct calls; `Subscribe<T>` and `Publish<T>` are empty |
| 3 | A fixed core, plus receivers that still subscribe and unsubscribe at run time: a diagnostic probe, a plug-in | `StaticFirst<&a>` | direct calls to the listed receivers, then the broker for the rest |

A type moves between the steps by that one line. What the move cannot do silently is enforced: a feature that needs
a subscription table does not compile for a `StaticTo` type (K27), a listed receiver that subscribes to the type must
accept it, a list nobody can receive from is rejected, and a debug build reports a subscriber the list forgot (K29).

### When the type cannot decide: explicit wiring

Step 2 needs receivers with static storage and one list per type. Outside that, the application binds receivers
itself, where it composes itself. Each form below exists for the case beside it; none is a second way to do steps 1
to 3. Receivers here are ordinary classes with a non-virtual `receive(const T&)`, and need no base class.

| Reach for | The real-world case | Its price |
|---|---|---|
| `wire(a, b)` | receivers that are locals or members, with no static storage for a type to name | one reference per receiver, held by the publisher: the cost of hand-written runtime binding |
| `StaticWiring<&a, &b>` | several independent wirings of one message type (two instances of a subsystem), or receivers that cannot be given a `Subscribe` base | none against hand-written calls |
| `publishCancelable(msg)` | a receiver that stops the rest of a publication on the direct-call path (it returns `bool`) | none (D5) |
| `Forward<Transport>`, `StaticForward<&transport>`, `publishFrom(...)` | a transport endpoint beside local receivers, with split horizon so ingress is not echoed back out | none against a hand-written endpoint |
| `BrokerPort<T>` | runtime subscribers behind an explicit wiring, with the broker's policies (Snapshot, a lock, a `Domain`) | the broker's |
| `Sink<T>` | a publisher that must not name its wiring: compiled into a library, or behind an interface that cannot be a template | one indirect call, the price of type erasure itself |

On the broker's side, `Route<T, Transport>` sends a type over a transport and can report rejection
(`PublishReport`), and `Domain<T>` gives a type isolated sessions.

### Recommended pattern

1. Write receivers as `Subscribe<T>` classes with `void receive(const T&) noexcept`, and publishers as `Publish<T>`
   classes that call `sub0::publish(*this, msg)`. Leave `override` off `receive()`: a wrong signature is a compile
   error on both paths without it, and the declaration then serves a brokered and a wired type alike.
2. Configure nothing until a type needs it. The default broker is the cheapest correct runtime dispatch (D2), and a
   debug build reports a publication that reaches nobody, a full table and a table changed during its own dispatch.
3. When a type's receivers are known and fixed, give them static storage and add `StaticTo` beside the type. The
   compiler names whatever must change. Where some receivers stay dynamic, use `StaticFirst`.
4. Use explicit wiring for the cases in the table above, and only for those.

The [examples](../examples/README.md) show each of these as a standalone program;
[promote_to_static](../examples/promote_to_static/main.cpp) builds one application at steps 1 and 2 from one source.

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
3. **Builtin:** the cheapest correct dispatch, with a table of `SUB0PUB_MAX_SUBSCRIPTIONS`.

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
| Topology | `StaticTo<&...>`, `StaticFirst<&...>` | brokered | direct calls to receivers with static storage, instead of (`StaticTo`) or before (`StaticFirst`) the subscription table; `StaticTo` gives up everything that needs the table (K27) |
| No receivers | `AllowNoReceivers`, `ReportNoReceivers` | reported in debug builds | whether a publication that reaches nobody is a failure; free for a publication that has receivers |
| Capacity | `Capacity<N>` | `SUB0PUB_MAX_SUBSCRIPTIONS` (8) | RAM only: the table is sized per type |
| Implementation | `Implementation<Broker>` | the library broker | a custom broker for a special case (for example one subscriber, one pointer of RAM) |

Every option that costs something is opt-in, and using a feature without opting in is caught: `cancel()`, routes,
publish reports and `filter()` do not compile without their option, on the broker and in a `StaticTo` list alike; invalid combinations (a lock without Snapshot,
Snapshot without a context, a lock with `StaticContext`, `Domain<T>` for a non-scoped type) are compile errors; table
changes during a Direct dispatch are reported by the debug-build check. [MIGRATION.md](../MIGRATION.md) lists the
opt-in for each v1 behaviour.

## Decisions

| # | Decision | Chosen | Rejected, with the measured reason |
|---|---|---|---|
| D1 | Where a type's broker policy lives | On the type (member alias, ADL declaration, `SUB0PUB_CONFIGURE`, `Tagged`), else the project header, else the builtin default | a broker chosen at each use site (sites can silently disagree); a central registry header of every type (dependency inversion); a macro per feature beside the project header (removed: a second way to set the same defaults, and one a single translation unit could set differently) |
| D2 | What the default costs | The cheapest correct dispatch: direct iteration, no publish context, no `filter()`, no lock | snapshot, `cancel()` and `filter()` always on: 77 / 287 instructions per publish against 38 / 101 for 1 / 8 subscribers |
| D3 | The hot-path structure | Direct calls to receivers bound at compile time: named by the type (`StaticTo`), or at the composition point (explicit wiring) | policy switches inside the virtual registry, which keep its dispatch model; routing static receivers through the registry: +78 to +86 publish instructions |
| D4 | The static-to-dynamic boundary | The runtime broker itself: `StaticFirst` on the type, or a `BrokerPort` in an explicit wiring | a second, policy-free slot registry (`DynamicPort`, removed): it duplicated the broker's default configuration, which already costs what a hand-written registry does (+3.5 / -1.5 publish instructions), without its capacity report, snapshot, lock or sessions; a registry in front of the static wiring: publish +37 (GCC) / +10 (Clang) over the hand-written equivalent |
| D5 | Cancellation on the static path | `bool` result with `publishCancelable` | a thread-local `cancel()` flag: GCC +3 instructions; Cortex-M33 +14 path instructions, +257 B RAM and a TLS dependency |
| D6 | Publisher spelling | `Publish<T>` with `sub0::publish()`, the type deciding the delivery; in explicit wiring the publisher holds its wiring by value, or a `Sink<T>` | a CRTP `Publisher` mixin (removed: it only renamed a wiring the publisher holds, made every publisher a template, and cost +6 path instructions on MSVC at 32 receivers); a CTAD factory (Clang +4 instructions, +24 B); C++23 deducing this (GCC 13 rejects it, and it costs the same as the mixin) |
| D7 | Lifetime | `Subscribe<T>` and `Publish<T>` have protected, non-virtual destructors; locked types subscribe with `trySubscribe()` after construction; `unsubscribe()` during a dispatch is safe under Snapshot; `Domain::close()` unsubscribes every subscriber, rejects new ones and quiesces | a virtual destructor (a vptr per object and an `operator delete` link dependency on small targets); subscribing in the base constructor for concurrent types (another thread could dispatch into a half-built object) |
| D8 | Teardown under concurrency | a sequentially consistent handshake: `unsubscribe()` waits only for a callback running on another thread | hazard pointers and epochs: cheaper only because they drop self-unsubscribe, nested-publish and thread-count safety, and they drop publications past their bounds in release builds |
| D10 | Where a type's topology lives | On the type, as a configuration option (`StaticTo`, `StaticFirst`), so `Subscribe<T>`, `Publish<T>` and `publish()` are written once and follow it; one topology per type, and `Tagged` makes a second type where a variation is needed | a topology chosen by each publisher or use site (sites can silently disagree, and a receiver cannot know which publisher will call it); a project-wide topology header that every unit must include (intrusive, build-system dependent, and a unit that misses it brokers the type silently); a second receiver and publisher vocabulary for wired types (the rewrite that kept applications on the broker); a second template parameter on `Subscribe` and `Publish` to select the wired form (it lengthened the RTTI name of every brokered `Subscribe<T>`: +4 B per message type; the wired forms are constrained specialisations instead) |
| D11 | A publication that reaches no receiver | A failure unless the type allows it: reported at run time on the broker (`SUB0PUB_NO_RECEIVERS_CHECK`, on in debug builds like the other contract checks), rejected at compile time for a `StaticTo` list; `AllowNoReceivers` opts a type out and `receiverCount()` lets a call site decide for itself | a silent no-op (it hides a publisher that starts before its subscribers, and a unit that does not see a type's configuration); a separate count before every dispatch (the check takes the place of the loop's own entry test instead, so a publication with receivers does not pay for it) |
| D12 | How wiring mistakes are found in a running program | One opt-in audit build (`SUB0PUB_AUDIT`) with a ledger per message type, fed by both delivery paths and reported at exit; the stopping checks (no receivers, unlisted subscriber) stay as the default for ordinary debug builds | more separate checks, one per mistake (each stops at the first failure and sees one side of it: the audit shows an unheard publication and the subscriber that arrived too late as the same event); recording in every build (a lock and a table per type on the hot path) |
| D9 | Language standard | C++23 is the contract; use concepts/requires where they simplify constraints | retaining C++17 compatibility scaffolding; adopting poorly supported features without toolchain and equal-work evidence (see "Language baseline" below) |

## Contracts

- **Subscriber lifetime.** After `unsubscribe()` returns, `receive()` is not called again, on any thread. When other
  threads may publish, call `unsubscribe()` from the most-derived destructor, before derived state is destroyed.
- **Subscription.** Unlocked subscribers subscribe in their constructor. Locked (concurrent) types do not: call
  `trySubscribe()` at the end of the most-derived constructor. A full table is reported
  (`SubscribeResult::CapacityExceeded`), never overrun.
- **Domains** must outlive the handles bound to them.
- **A type's topology.** A `StaticTo` or `StaticFirst` list borrows the receivers it names: they have static storage,
  and publishing before one is constructed or after it is destroyed is an ordering error of the application, as it is
  for `StaticWiring`. Delivery is in the order of the list; the runtime subscribers of a `StaticFirst` type follow in
  the order they subscribed. A subscriber that a `StaticTo` list does not name is never called (K29).
- **Unheard publications.** A publication that reaches no receiver is reported unless its type is configured with
  `AllowNoReceivers`. A closed `Domain` drops a publication without a report: its session has ended.
- **Wirings add no synchronisation.** Concurrent publishers on one wiring, or of one `StaticTo` type, need
  thread-safe receivers. A `StaticFirst` type's runtime side is the broker, with the type's own configuration.
- **Transports.** `Forward` ignores send results; `Route` records them. Neither means remote delivery. Split horizon
  prevents an immediate echo to the link a message arrived from, not arbitrary network cycles.

## Known limitations

Each limitation has a measured price or a documented usage rule. Identifiers are stable, so tests and comments can
refer to them; a number that is missing was retired with its limitation (K21 went with `DynamicPort`).

| # | Limitation | Price | Route to removing it |
|---|---|---|---|
| K1 | Teardown safety on create + destroy: a subscribed flag and, with a publish context, a check for dispatches in progress | create + destroy 47 instructions in the default (v1.0: 48; GCC, compare-v1-v2 report) | skip the check when no dispatch of the table is active on this thread |
| K2 | Types with a publish context carry a dispatch frame (origin, report, snapshot) even without routes | Snapshot, 1 subscriber: +2; Direct, 0 subscribers: +7 | a minimal frame for types without routes |
| K3 | Locked types pay a handshake per subscriber per publish, and a second lock acquisition | `std::mutex`, 1 / 8 subscribers: 260 / 554 instructions per publish | per-subscriber reference counts or epochs, if they can pass the same lifetime tests |
| K4 | Concurrent `unsubscribe()` blocks for at most one callback on another thread; two receivers unsubscribing each other at once from different threads deadlock | a usage rule | a non-blocking `unsubscribeLater()` for use inside receivers |
| K5 | Locked types need an explicit `trySubscribe()` after construction | easy to forget | a CRTP helper that subscribes after construction |
| K6 | `Domain` lifetime is only debug-checked; configuration consistency across translation units is a build contract with a best-effort check | undefined behaviour if violated in a release build | link-time detection |
| K8 | `Implementation<>` brokers support global storage only | `Domain` needs the library broker | make the table type part of the broker concept |
| K9 | `cancel()`, re-entrancy checks and teardown walk this thread's dispatch frames | O(nesting depth), usually 1 | per-table frame chains if deep nesting appears |
| K10 | The cross-thread teardown test is probabilistic | a removed wait is caught in about 4 of 5 runs | a deterministic interleaving harness |
| K13 | C++23 feature coverage varies across host and embedded compilers | selecting C++23 mode does not provide every language/library feature | validate each newly used feature in the supported matrix |
| K14 | Explicit wiring routes by capability and is silent on a signature mismatch: a receiver whose `receive` does not accept the message is skipped without a diagnostic. A `StaticTo` list is checked: a listed receiver that derives from `Subscribe<T>` must accept `T` | a missed delivery, found only by tests | state it where it is bound: `static_assert(sub0::handles_v<R, T>)` |
| K15 | Split horizon is decided at compile time when the origin's type is bound once; the origin must then be that endpoint | a debug assertion | `publishFrom<Endpoint>(msg)` identifies the origin by type |
| K16 | A lock requires `ThreadLocalContext`, so concurrent configurations need TLS | `__aeabi_read_tp` on Cortex-M | a context keyed by the RTOS thread |
| K17 | Nothing is shared between message types: each instantiates its own table and dispatch loop | Cortex-M33, per further type: +220 B text and +49 B RAM (default), +386 B and +53 B (Snapshot, context, filter) | a type-erased dispatch core shared by types with the same configuration |
| K18 | `wire(...)`: split horizon between two endpoints of the same transport type is an address compare | publish +8 (GCC) / +10 (Clang) | give each link its own adapter type and use `publishFrom<Link>(msg)` |
| K19 | `false` from a receiver stops only `publishCancelable`; plain `publish()` ignores it, and `Sink<T>` has no cancelable publish | a missed stop | a diagnostic when `publish()` reaches a bool-returning receiver |
| K20 | `StaticWiring` and `StaticTo` need static storage: no locals; array-element NTTP support varies by compiler; a bound array binds nothing, and fan-out is unrolled | 32 receivers: +668 B Cortex-M33 text against a hand-written loop | `wire(...)` for dynamic lifetimes; an array binding that delivers by loop |
| K22 | A nested publication on the same static wiring compiles as recursion through the fold | GCC x86 +536 B text; Clang +4 instructions; Cortex-M33 none | publish nested messages through a separate wiring |
| K23 | Clang does not propagate `Wiring` bindings held inside an aggregate as it does a struct of pointers | publish +3 to +5, RAM +24 B (Clang only) | open |
| K24 | Cancellation combined with `filter()` is not byte-identical to hand-written code | Cortex-M33 +4 path instructions, +12 B; GCC +3 | open (small) |
| K25 | `Publish<T>` has a protected destructor, so a publisher is always a derived class | one line per publisher type | a library-provided final handle type |
| K27 | A `StaticTo` type has no subscription table: `isSubscribed()`, `trySubscribe()`, `unsubscribe()`, `cancel()`, `Domain`, `Route` and publish reports do not compile for it | a type that needs one stays on the broker, uses `StaticFirst`, or uses explicit wiring (`publishCancelable`, `Forward`) | a static meaning for the queries (membership of the list), and cancellation that reads the same on both paths |
| K28 | A translation unit that publishes a `StaticTo` or `StaticFirst` type needs the definition of every receiver the list names (a compile error otherwise) | publishers depend on the receivers' headers at compile time; nothing at run time | an out-of-line delivery function per type, which trades the dependency for one call where LTO is off |
| K29 | A subscriber that its type's `StaticTo` list does not name is never called, and only the debug-build check (`SUB0PUB_UNLISTED_CHECK`) reports it, when it is constructed | a missed delivery in a release build | a link-time diagnostic |
| K30 | The receivers a `StaticFirst` list names are broker subscribers by type: each keeps its virtual table and subscription state, although it is called directly and never entered in the table | two listed receivers, against `StaticWiring` with a `BrokerPort` and plain receivers: RAM +80 B (GCC) / +104 B (Clang) / +112 B (MSVC); publish equal on Clang and MSVC, +27 instructions on GCC where it would otherwise devirtualise a program's only subscribing class ([EVIDENCE.md](EVIDENCE.md)) | explicit wiring with a `BrokerPort`, where that matters |
| K31 | The no-receivers report is a run-time check for brokered types, on by default only in debug builds | a release build still publishes to nobody silently unless `SUB0PUB_NO_RECEIVERS_CHECK` or `ReportNoReceivers` says otherwise | none planned: the default follows the library's other contract checks |
| K32 | Topology belongs to the type, not to a publisher: a publisher cannot choose another delivery (a queue, an asynchronous fan-out) for a type it publishes from a hot loop | a second type, through `Tagged<Payload, Tag>` | publisher-side topology, when a real case needs it |
| K33 | The audit describes one run and has blind spots: explicit wirings are not recorded; a `Scoped` type's sessions share one ledger; a subscriber counts as reached when delivery gets to it, whatever `filter()` or `cancel()` then do; 16 receivers and 8 publisher classes are remembered per type; a subscriber of a `StaticTo` type that its list does not name is shown by address only, and so is every runtime subscriber without RTTI | a finding it cannot make, never a wrong program: the audit changes no behaviour | per-`Domain` ledgers; recording `wire()` and `StaticWiring` |
| K34 | The stream serialisation layer (`sub0pub/ipc/`) keeps its v1 design: its own virtual `IStream` / `OStream` beside `SUB0PUB_STD`, CRTP `ForwardSubscribe` / `ForwardPublish` whose name collides with the wiring's unrelated `Forward<Transport>`, a reader that publishes a frame only after its postfix, and raw in-memory payloads | two meanings of "forward" to learn; one virtual call per stream operation | a redesign of the serialisation API against the converged vocabulary, with its own evidence |
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
| Result enums, `const T&` payloads | kept | `std::expected` suits a future admission boundary, not synchronous publication; queue ownership belongs in an adapter ([INTEGRATION.md](INTEGRATION.md)) |
| Synchronisation and storage | unchanged | their lifetime and cost contracts do not depend on the language mode |

## Compilation cost

The library is header-only, so parsing and instantiation repeat in every consumer translation unit. Build time is
measured separately from runtime cost ([COMPILE_TIME.md](COMPILE_TIME.md)):

- Aggregate arity detection selects the recursive type before requesting its value, so only one branch of its binary
  search is instantiated at each step (32 members at most, unchanged).
- The narrow broker headers do not include `<algorithm>`: snapshots copy pointer arrays with `memcpy`, and
  `Domain::close()` records the subscribers it unsubscribes in its clearing pass, only where the single-threaded path uses them.
- `<thread>` stays in the broker headers: any message type may opt in to a lock, whatever the global defaults say.
- No precompiled headers, modules, unity builds or a type-erased broker core: each trades portability, integration or
  runtime cost, and needs its own equal-work evidence.

The IPC buffer registry stays a fixed-capacity sorted array with binary lookup. `trySet()` reports a full registry
before moving entries or touching padding, and replacement succeeds when full; this is work on the path that sets a publisher's buffer, not on the publish
path.
