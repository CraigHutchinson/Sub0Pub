# v1 to v2 Migration Guide

This document tracks all breaking changes between Sub0Pub v1 and v2. Update this document with any commit that introduces a migration-relevant change.

For runnable static, dynamic and mixed-path migration recipes, see [examples](examples/README.md).
[docs/USAGE.md](docs/USAGE.md) explains which delivery to choose: the runtime broker, a message type that names its
receivers, or explicit wiring.

---

## Changes since v2.0

These apply to code already on v2. Each says what to do.

### A publication that reaches no receiver is reported

Publishing a `Data` type that nobody is subscribed to used to be a silent no-op. It is almost always a mistake (a
publisher that starts before its subscribers, a subscriber that was never constructed, a translation unit that does
not see a type's configuration), so it is now detected: `SUB0PUB_NO_RECEIVERS_CHECK` is on in debug builds and calls
`SUB0PUB_NO_RECEIVERS(what)`, whose default asserts and then aborts. Release builds are unchanged unless you define
the check `true`; it replaces the dispatch loop's own entry test, so a publication that has receivers does not pay
for it.

| | Every type | One type |
|---|---|---|
| Report an unheard publication in every build | `SUB0PUB_NO_RECEIVERS_CHECK=true` | `sub0::ReportNoReceivers` |
| Never report it | `SUB0PUB_NO_RECEIVERS_CHECK=false` | `sub0::AllowNoReceivers` |

A closed `Domain` still drops a publication without a report: its session has ended. A `BrokerPort<T>` publishes
into the broker like any publisher, so a wiring whose runtime side may be empty needs `AllowNoReceivers` on `T`.

**Action:** Build in debug and run. Where a publication is reported:
- if a receiver should have been there, fix the order in which the publisher and its subscribers are created;
- if an absent receiver is expected for that type (a diagnostic stream, a plug-in loaded at run time), say so on
  the type: `using sub0_config = sub0::config<sub0::AllowNoReceivers>;`. A call site can still ask how many
  receivers there are with `sub0::receiverCount<Data>(publisher)` and decide for itself;
- to keep the old behaviour everywhere, define `SUB0PUB_NO_RECEIVERS_CHECK=false`.

### `disconnect()` renamed to `unsubscribe()`

`Subscribe<Data>::disconnect()` was the counterpart of `trySubscribe()` and `isSubscribed()` under another word. It
is now `unsubscribe()`, and an application-defined broker (`Implementation<Broker>`) implements
`unsubscribe(Subscribe<Data>*)`. Its behaviour and its teardown contract are unchanged.

**Action:** Rename the calls. There is no alias for the old name.

### Unused helpers removed

- `SUB0PUB_TRACE`: the macro only included `<iostream>`; nothing was ever traced. The audit build (below) is the
  diagnostic that exists.
- `SUB0PUB_STRINGIFY`, `SUB0PUB_STRINGIFY_HELPER`: unused.
- `sub0pub/utility/traits.hpp` (`sub0::utility::is_detected`, `detected_t`, `detected_or_t`, `nonesuch`) and
  `sub0::forward_receiver_t`: a C++17 detection idiom with one use, now a requires-expression.
- `sub0::utility::sizeOf<>()` and `copyTo()`: unused.
- `sub0pub/sub0pub.hpp` no longer includes `<algorithm>`, `<array>`, `<stdexcept>`, `<thread>`, `<tuple>` and the
  other standard headers the single-file v1 header happened to provide.

**Action:** Include the standard headers your own code uses. The rest need action only if you used them directly.

### The audit build: `SUB0PUB_AUDIT` (new)

Defining `SUB0PUB_AUDIT` true for the whole program makes it record what it publishes and delivers, through the
runtime broker and through `StaticTo` / `StaticFirst` lists, and report at exit: publications that reached no
receiver and who made them, subscribers that were never called, refused subscriptions, subscribers a `StaticTo` list
does not name, and which brokered types had receivers that never changed (candidates for `StaticTo`).
`sub0::auditFindings()` returns the number of findings and `sub0::auditReport()` writes the report on request; in a
build without the macro both are the constant 0 and nothing else exists. `SUB0PUB_AUDIT_PRINT(line)` says where the
report goes (default: stderr) and `SUB0PUB_AUDIT_EXIT(findings)` what follows the report at exit (default: nothing).

**Action:** None; it is opt-in. In an audit build `SUB0PUB_NO_RECEIVERS_CHECK` and `SUB0PUB_UNLISTED_CHECK` default
to off, so that a run completes and the audit reports everything they would have stopped at. How to read the report:
[docs/USAGE.md](docs/USAGE.md#finding-wiring-mistakes-the-audit-build).

### A Data type can name its receivers: `StaticTo` and `StaticFirst` (new)

A type's configuration can now say that it is delivered to fixed receivers by direct calls:

```cpp
class Display;
extern Display display;
struct Reading { int celsius; using sub0_config = sub0::config<sub0::StaticTo<&display>>; };
```

`Subscribe<Reading>`, `Publish<Reading>`, `SubscribeAll` and `sub0::publish()` follow it, so the code that
subscribes and publishes is the code you already have. For a `StaticTo` type `Subscribe` is an empty base and
`Publish` an empty handle: no subscription table, no run-time subscription, no virtual call. `StaticFirst<&a>` calls the
listed receivers directly and then publishes through the runtime broker to whoever subscribed at run time.

**Action:** None; it is opt-in per type. To move an existing type onto direct calls:
- give its receivers static storage, declare them before the type, and add the option beside the type;
- remove `override` from the `receive()` of that type's subscribers: the base has no virtual function to override.
  A `receive()` that does not accept `const Data&` is still a compile error, in both forms;
- include the receivers' definitions in every translation unit that publishes the type (a compile error tells you);
- `unsubscribe()`, `trySubscribe()`, `isSubscribed()`, `cancel()`, `Route`, `Domain` and publish reports need a
  subscription table and do not compile for a `StaticTo` type; a `filter()` needs the type's `sub0::Filter`, as it
  does on the broker, and the signature the broker calls (`bool filter(const Data&) noexcept`, not `const`), so that
  it filters the same way if the type returns to the broker.

Delivery is in the order of the list, where the broker delivers in the order subscribers subscribed. A subscriber
the list does not name is never called: `SUB0PUB_UNLISTED_CHECK` (debug builds) reports its construction through
`SUB0PUB_UNLISTED_RECEIVER(what)`. A list in which nobody can receive the type is a compile error unless the type is
also `sub0::AllowNoReceivers`. Which form to use, and when the explicit `wire()` level is the right one instead:
[docs/USAGE.md](docs/USAGE.md).

### `Publisher<Derived, Out>` removed

The CRTP mixin gave a publisher `this->publish(msg)` over a wiring it held. A publisher no longer needs to know
its output at all: it derives from `Publish<T>` and its message type's configuration decides the delivery.

**Action:** Prefer the type's configuration. Where a publisher must hold an explicit wiring (receivers without
static storage, several wirings of one type), hold it as a member:

```cpp
// before
template<class Out>
struct Sensor : sub0::Publisher<Sensor<Out>, Out>
{
    using sub0::Publisher<Sensor<Out>, Out>::Publisher;
    void measure(int v) noexcept { this->publish(Sample{v}); }
};

// after: the type decides (preferred)
struct Sensor : sub0::Publish<Sample>
{
    void measure(int v) noexcept { sub0::publish(*this, Sample{v}); }
};

// after: an explicit wiring, held by value
template<class Out>
struct Sensor
{
    Out out;
    void measure(int v) noexcept { out.publish(Sample{v}); }
};
```

### `DynamicPort<T, N>` removed

`DynamicPort` was a second, policy-free registry for runtime subscribers behind a static wiring. The runtime broker
in its default configuration already measures at a hand-written registry's cost, and it has what `DynamicPort`
lacked: a reported capacity, safe removal during delivery with `Snapshot`, a lock, sessions.

**Action:** Subscribe runtime receivers with `Subscribe<T>`. Beside receivers with static storage, configure the
type with `sub0::StaticFirst<&fixed>`; in an explicit wiring, bind a `BrokerPort<T>`:

```cpp
// before
sub0::DynamicPort<Sample, 4> port;
auto bus = sub0::wire(controller, port);
port.add(&probe);                                    // struct Probe : sub0::DynamicPort<Sample, 4>::Receiver

// after: the type decides (preferred; `controller` has static storage)
struct Sample { int v; using sub0_config = sub0::config<sub0::Capacity<4>, sub0::StaticFirst<&controller>>; };
Probe probe;                                         // struct Probe : sub0::Subscribe<Sample>

// after: an explicit wiring
sub0::BrokerPort<Sample> port;
auto bus = sub0::wire(controller, port);
Probe probe;                                         // struct Probe : sub0::Subscribe<Sample>
```

`tryAdd()` becoming `false` is `isSubscribed()` becoming `false`; `remove()` is `unsubscribe()`, or the end of the
subscriber's lifetime.

### `sub0pub/wiring/publisher.hpp` renamed to `sub0pub/wiring/sink.hpp`

The header now holds `Sink<T>` alone.

**Action:** Only if you included that header directly rather than `sub0pub/wiring.hpp` or `sub0pub/sub0pub.hpp`.

### `SubscribeAll` packs empty bases on the MSVC ABI

`SubscribeAll` is declared with `SUB0PUB_EMPTY_BASES` (`__declspec(empty_bases)` on the MSVC ABI, nothing elsewhere),
so that a receiver of statically wired types is as small as a class without the bases. The layout of a class whose
`Subscribe` bases are all runtime subscribers does not change: none of them is empty.

**Action:** None.

### The configuration fingerprint covers the topology and the no-receivers policy

`SUB0PUB_CHECK_CONFIG` now also reports a type that one translation unit wires statically and another does not, at
the construction of the second unit's first subscriber or publisher of that type.

**Action:** None.

---

## Build Requirements

| | v1 | v2 |
|---|---|---|
| C++ Standard | Claims C++11, actually needs C++17 | C++23 (transitive CMake requirement) |
| CMake Minimum | 3.7.1 | 3.21 |
| Build | `./configure && cmake --build ./build` | `cmake --preset default && cmake --build --preset default` |
| Test | N/A (no tests) | `ctest --preset default` |

**Action:** Link `Sub0Pub::Sub0Pub` to inherit `cxx_std_23`. For direct header use, enable C++23 mode
(`-std=c++23`, or the C++23/latest mode supported by your MSVC toolchain). C++17/C++20 builds are no longer
supported and receive a header diagnostic. CMakePresets remain the recommended configure/build/test entry point.

The language mode changes nothing you call: delivery, filtering, cancellation and `Sink` copying behave as before.
`StaticWiring` may now bind array elements (`StaticWiring<&receivers[0], &receivers[1]>`) on compilers that accept
subobject addresses as template arguments; for code that must build on every supported compiler, bind named objects
or use `wire(receivers[0], receivers[1])`. Binding a whole array still binds nothing.

---

## API Changes

### `Broker` moved to `sub0::detail`

The `Broker<Data>` class is now in `sub0::detail::Broker<Data>`. It was never intended as public API.

**Action:** If you referenced `sub0::Broker<Data>` directly, change to `sub0::detail::Broker<Data>`. Consider refactoring to avoid direct Broker access.

### `cancel()` free function signature changed

```cpp
// v1: required a dummy data argument for template deduction
template<typename From, typename Data>
void cancel(From& from, const Data& data);

// v2: Data type is now an explicit template parameter
template<typename Data, typename From>
void cancel(From& from);
```

**Action:** Change `sub0::cancel(obj, someData)` to `sub0::cancel<DataType>(obj)`.

### `receive()` and `filter()` are now `noexcept`

```cpp
// v1
virtual void receive(const Data& data) = 0;
virtual bool filter(const Data& data);

// v2
virtual void receive(const Data& data) noexcept = 0;
virtual bool filter(const Data& data) noexcept;
```

**Action:** Add `noexcept` to all `receive()` and `filter()` overrides. If your receive handlers can throw, wrap them in try/catch.

### `publish()` is now `noexcept`

The free functions `sub0::publish()` and `Publish<Data>::publish()` are now `noexcept`.

**Action:** Ensure no exceptions propagate out of subscriber `receive()` callbacks.

### `Subscribe<Data>::isSubscribed()` and `trySubscribe()` added

New `sub0::SubscribeResult` enum (`Subscribed`, `CapacityExceeded`). `Subscribe<Data>::isSubscribed()` reports whether the subscriber is in the per-type table. `Subscribe<Data>::trySubscribe()` retries registration and returns a `SubscribeResult`, leaving the table unchanged when it is full.

**Action:** None required. Check `isSubscribed()` where the number of subscribers per type cannot be bounded ahead of time.

### `Subscribe<Data>` and `Publish<Data>` have no virtual destructor

`Subscribe<Data>::~Subscribe()` and `Publish<Data>::~Publish()` are `protected` and non-virtual. `Subscribe<Data>` still has virtual delivery callbacks; `Publish<Data>` is non-polymorphic and, for global storage, an empty handle. A subscriber or publisher is destroyed as its own type. Deleting one through a `Subscribe<Data>*` or `Publish<Data>*` is a compile error, where v1 destroyed it virtually.

Because its destructor is protected, a `Publish<Data>` is always a base: declare a publisher as a class derived from it, not as a `sub0::Publish<Data>` variable.

This removes the deleting-destructor code and the `operator delete` link dependency from small targets.

**Action:**
- Delete or hold subscribers and publishers by their own type.
- Replace a `sub0::Publish<Data>` variable with a derived type, e.g. `struct DataOut final : sub0::Publish<Data> { using Publish::Publish; };`.
- Mark leaf subscriber classes `final` when they are destroyed through a pointer to their own type (`delete`, `std::unique_ptr`) or held in `std::optional`. Otherwise gcc (`-Wdelete-non-virtual-dtor`) and clang (`-Wdelete-non-abstract-non-virtual-dtor`) warn.
- Remove `override` from subscriber destructors.

### `SUB0PUB_THREAD_SAFE` subscribers subscribe explicitly

With a lock (`SUB0PUB_THREAD_SAFE`, or `sub0::LockWith<L>`), the `Subscribe<Data>` constructor no longer subscribes the subscriber. Subscribing there would let another thread call `receive()` before the derived class is constructed. After `unsubscribe()` returns, `receive()` is not called again on any thread.

**Action:** In the most-derived subscriber:
- call `trySubscribe()` at the end of the constructor;
- call `unsubscribe()` at the start of the destructor.

```cpp
struct Logger final : sub0::Subscribe<Sample>
{
    Logger() noexcept { trySubscribe(); }
    ~Logger() { unsubscribe(); }
    void receive(const Sample&) noexcept;
};
```

Single-threaded configurations are unchanged: they subscribe in the constructor.

### `SubscribeResult::Closed` added

`trySubscribe()` returns `Closed` when a scoped subscriber's `Domain` has been closed (see "Per-type configuration" below).

**Action:** A `switch` over `SubscribeResult` needs the new case.

### `Subscribe<Data>::cancel()` and `sub0::cancel<Data>()` are `noexcept`

`Subscribe<Data>::cancel()` is now `const noexcept`, and the free function `sub0::cancel<Data>(from)` is `noexcept`. `cancel()` outside a publication of that type does nothing; it no longer asserts.

**Action:** None.

### `detail::Broker<Data>` is an alias of the configured broker

`sub0::detail::Broker<Data>` now names the broker implementation that `Data`'s configuration selects; `cMaxSubscriptions` is still available. The broker's `unsubscribe()`, `isSubscribed()`, `active()`, `typeId()` and `typeName()` members are gone: use `Subscribe<Data>` and `Publish<Data>` instead.

**Action:** Remove direct `detail::Broker` use.

### Per-type configuration (new)

Each `Data` type can now choose its own policy. The `SUB0PUB_*` macros are the default for types that don't. The configuration is resolved once per type, from exactly one of:
- a member alias: `struct Imu { ...; using sub0_config = sub0::config<sub0::Capacity<2>, sub0::NoFilter>; };`
- an ADL declaration in the type's namespace: `sub0::config<sub0::Direct> sub0_config(Gps*);`
- `SUB0PUB_CONFIGURE(Type, options...)`, for types you cannot modify;
- otherwise the project default (`SUB0PUB_CONFIG_HEADER`), otherwise the `SUB0PUB_*` macros.

The options are:
- `Capacity<N>`;
- `Snapshot` (which also selects `ThreadLocalContext` unless a context is already chosen), `Direct` or `DirectChecked`;
- `ThreadLocalContext`, `StaticContext` (no TLS) or `NoContext`;
- `LockWith<L>`, which also selects `Snapshot` and `ThreadLocalContext`;
- `Filter` or `NoFilter`;
- `Scoped`, with `Domain<Data>` sessions;
- `Implementation<Broker>`, for an application-defined broker;
- `StaticTo<&...>` or `StaticFirst<&...>`, which wire the type to fixed receivers (since v2.0, above);
- `AllowNoReceivers` or `ReportNoReceivers` (since v2.0, above).

`Route<Data, Transport>` binds a transport endpoint to a table. `sub0::publish(from, data, report)` reports what each route accepted. Invalid combinations are compile errors.

Every translation unit must resolve the same configuration for a type: resolving it differently is an ODR violation. A debug-build check (`SUB0PUB_CHECK_CONFIG`) reports mismatches it observes. Types local to one translation unit may use different `SUB0PUB_*` macros in different units: `sub0::config<Opts...>` is an alias of `sub0::with<Default, Opts...>`, so it names a different type wherever the default differs.

**Action:** None for the mechanism itself; see "The default is the cheapest dispatch" below for what the default now includes. Options and what each costs: [docs/DESIGN.md](docs/DESIGN.md#per-type-configuration-of-the-runtime-broker).

### Explicit wiring (new)

`sub0::wire(a, b, logger)` and `sub0::StaticWiring<&a, &b, &logger>` bind receivers at the application's composition point, for the cases a message type cannot express with `StaticTo` (above; [docs/USAGE.md](docs/USAGE.md)). Receivers are plain classes with a non-virtual `receive(const T&)`; each delivery is a direct call, measured equal to hand-written code. The rest of the static wiring API:
- `publishCancelable()` stops at a receiver whose `bool receive()` returns `false`;
- `Sink<T>` is a type-erased port for non-template publishers;
- `Forward<Transport>` and `StaticForward<&transport>` are transport endpoints, with split horizon through `publishFrom()`;
- `BrokerPort<T>` brings runtime subscribers into an explicit wiring;
- `handles_v<R, T>` asserts that a receiver handles a message.

v2.0 also had a `Publisher<Derived, Out>` mixin and a `DynamicPort<T, N>`; both were removed afterwards (above).

**Action:** None. Measured forms: [docs/EVIDENCE.md](docs/EVIDENCE.md).

### `BufferRegister::trySet()` reports a full IPC registry (new)

`trySet(header, buffer)` returns `false` for a new entry when the registry is full, leaving the registry and the
buffer's padding bytes untouched; replacing an existing entry still succeeds when full. `set()` keeps its signature:
it asserts on overflow when assertions are enabled, and otherwise drops the new entry (v1 wrote past the array).

**Action:** None. Call `trySet()` where a full registry must be handled.

### The library is split into focused headers

`include/sub0pub/sub0pub.hpp` was a single 3,200-line file. It is now an umbrella header over one header per
responsibility, grouped in `utility/`, `broker/`, `wiring/` and `ipc/`, with an entry header per area:
`sub0pub/broker.hpp` (runtime broker), `sub0pub/wiring.hpp` (explicit wiring, no broker), `sub0pub/ipc.hpp` (IPC
serialisation, no broker), and the bridges `sub0pub/wiring/broker_port.hpp` and `sub0pub/ipc/forward.hpp`. The
`SUB0PUB_*` defaults live in `sub0pub/config_macros.hpp`. No name, namespace or behaviour changed: the generated
code is identical (collapse evidence, `tests/collapse/budgets.json`).

**Action:** None. `#include <sub0pub/sub0pub.hpp>` works as before, including the standard headers it provided. To
compile less, include only the part you use (README, "Headers"). Headers under the area directories other than
the entry headers are implementation structure and may move. The narrow broker headers do not include
`<algorithm>`: a translation unit that uses standard algorithms includes it itself (the umbrella still does).

---

## Behavioral Changes

### Subscription capacity is a reported outcome, not an assertion

In v1 and earlier v2, exceeding `SUB0PUB_MAX_SUBSCRIPTIONS` only triggered `assert()`. With `NDEBUG` the broker wrote past its fixed table, and the next `publish()` overflowed its snapshot buffer. Now the extra subscriber is constructed but not registered: `isSubscribed()` returns `false`, it never receives data, and destroying it is a no-op. This is the same in debug and release builds. The capacity assertion has been removed, so debug builds no longer abort.

**Action:** Code that relied on the debug assertion to detect over-subscription should check `isSubscribed()` instead.

### Subscription order preserved on removal

In v1, `unsubscribe()` used swap-with-last, silently reordering the subscriber list. In v2, order is preserved using `std::move` of the tail.

**Action:** No code change needed. If you relied on the v1 reordering behavior (unlikely), be aware that order is now stable.

### Publish cancel flag redesigned

In v1, `publishCanceled_` was a per-`Broker`-instance `mutable bool`. In v2, it is a `thread_local bool` scoped to the active publish invocation, with save/restore for re-entrant calls.

**Action:** No code change needed.

### `Publish::publish()` is now `protected`

The member function `Publish<Data>::publish(data)` is no longer public. Use the free function `sub0::publish(this, data)` from derived classes.

**Action:** Replace `myPublisher.publish(data)` with `sub0::publish(myPublisher, data)`, or call `publish(data)` from within the derived class (protected access).

### The default is the cheapest dispatch; costly features are opt-in and detected

A type without its own configuration dispatches with the cheapest correct loop: direct iteration over the table, no
publish context, no `filter()`, no lock. Each feature that costs something is opt-in, per type or for every type.
Using one without opting in is caught at compile time or by a debug-build check:

| Feature | Opt in for every type | Per type | Without it |
|---|---|---|---|
| Subscribe or unsubscribe a type from its own `receive()` (including destroying the subscriber) | `SUB0PUB_REENTRANT_SAFE` | `sub0::Snapshot` | Reported in debug builds (`SUB0PUB_REENTRANT_CHECK`) |
| `cancel()`, `Route`, publish reports | `SUB0PUB_CANCEL` | `sub0::ThreadLocalContext` (or `StaticContext`) | Compile error naming the opt-in |
| `filter()` | `SUB0PUB_FILTER` | `sub0::Filter` | Compile error: a subscriber declaring `filter()` does not compile, even without `override` |
| Publishing or subscribing from several threads at once | `SUB0PUB_THREAD_SAFE` | `sub0::LockWith<L>` | Reported in debug builds (`SUB0PUB_THREAD_CHECK`) |

Earlier v2 builds defaulted to snapshot dispatch with `cancel()` and `filter()` always available.

A nested publish of the same type from `receive()` is supported under every dispatch. `SUB0PUB_REENTRANT_CHECK` no
longer reports it; it reports only a change to the table being dispatched.

**Action:** Build in debug and fix what is reported:
- add `SUB0PUB_FILTER` or `sub0::Filter` where `filter()` is overridden;
- add `SUB0PUB_CANCEL` or `sub0::ThreadLocalContext` where `cancel()` is called;
- add `SUB0PUB_REENTRANT_SAFE` or `sub0::Snapshot` where a subscriber of a type is created or destroyed, or its
  `Domain` closed, from that type's `receive()` or `filter()`;
- add `SUB0PUB_THREAD_SAFE` or `sub0::LockWith<L>` where a type is used from several threads at once.

To restore the earlier v2 behaviour for every type, define `SUB0PUB_REENTRANT_SAFE`, `SUB0PUB_CANCEL` and
`SUB0PUB_FILTER` as `true`.

### Unsubscribing during a dispatch is safe with Snapshot dispatch

A subscriber unsubscribed or destroyed while its type is being dispatched on the same thread is removed from that dispatch. Before, the dispatch's snapshot still held it and called it afterwards (issue #5). `filter()` may also unsubscribe or destroy its own subscriber: `receive()` is then not called.

Closing a `Domain` from a dispatch of its own table is a table change too. With Snapshot dispatch it is supported, and no subscriber of that domain receives the rest of the publication. With `DirectChecked` it is reported (a debug build aborts by default); if the handler returns, no subscriber receives the rest of the publication.

Each publication's `cancel()` and the `DirectChecked` re-entrancy check apply only to the table being dispatched. That is one per type, or one per `Domain`.

**Action:** None.

### `SUB0PUB_TYPEIDNAME` compiles

`SUB0PUB_TYPEIDNAME` did not compile (issue #11). It does now: the identity passed to a `Subscribe`/`Publish` constructor names the type in stream headers. `Publish<Data>::typeName()` and `typeId()` are now public. Publishers and subscribers of one type may be constructed on several threads at once.

**Action:** None.

### Type ID fallback uses compile-time hash

In v1, all types without `SUB0PUB_TYPEIDNAME` received a hardcoded ID of `12345`, making IPC with multiple types silently broken. In v2, `utility::typeHash<Data>()` generates a unique compile-time hash per type using `__PRETTY_FUNCTION__` / `__FUNCSIG__`.

**Action:** No code change needed. IPC now works correctly without `SUB0PUB_TYPEIDNAME`. Note that type hashes are not stable across different compilers — use `SUB0PUB_TYPEIDNAME` for cross-compiler IPC.

---

## Removed Features

### `SUB0_EXPERIMENTAL` removed

The `SUB0_EXPERIMENTAL` flag and `publish_cstatic()` have been removed.

**Action:** If you used `sub0::publish_cstatic(data)`, create a static publisher yourself:
```cpp
struct DataOut final : sub0::Publish<Data> {};
static DataOut publisher;
sub0::publish(publisher, data);
```

### `SUB0_BROKERSTATE` macro removed

The commented-out `SUB0_BROKERSTATE` macro has been removed. C++17 `inline static` handles this.

**Action:** Remove any `SUB0_BROKERSTATE` invocations.

---

## Configuration Changes

### `SUB0PUB_MAX_SUBSCRIPTIONS` (new in v1.0, carried to v2)

The fixed `cMaxSubscriptions = 8` is now configurable via `#define SUB0PUB_MAX_SUBSCRIPTIONS N` before including the header.

### `SUB0PUB_THREAD_SAFE` (new in v1.0, carried to v2)

Define `SUB0PUB_THREAD_SAFE true` to enable mutex-guarded subscribe/unsubscribe/publish operations.

### `SUB0PUB_REENTRANT_SAFE` (new in v2)

Default `false`. With `true`, `publish()` snapshot-copies the subscriber list before dispatching, so a subscriber may subscribe or unsubscribe (or destroy) a subscriber of the same type from within `receive()`. It also gives every type a publish context (`cancel()`).

### `SUB0PUB_CANCEL`, `SUB0PUB_FILTER` (new in v2)

Default `false`. `SUB0PUB_CANCEL` gives every type a publish context: `cancel()`, `Route` and publish reports. `SUB0PUB_FILTER` gives every subscriber a `filter()`. Per type: `sub0::ThreadLocalContext`, `sub0::Filter`.

### `SUB0PUB_THREAD_CHECK` and `SUB0PUB_THREAD_VIOLATION` (new in v2)

Without a lock, a `Data` type must not be published, subscribed or unsubscribed from two threads at once. `SUB0PUB_THREAD_CHECK` (default: on without `NDEBUG`) detects such an overlap and calls `SUB0PUB_THREAD_VIOLATION(what)`, which by default asserts, then aborts. It uses a `thread_local`; define it `false` on a target without thread-local storage.

### `SUB0PUB_REENTRANT_CHECK` and `SUB0PUB_REENTRANT_VIOLATION` (new in v2)

Without snapshot dispatch, a `receive()` that subscribes or unsubscribes (or destroys) a subscriber of its own `Data` type on the same thread is unsupported. `SUB0PUB_REENTRANT_CHECK` detects it and calls `SUB0PUB_REENTRANT_VIOLATION(what)`, which by default asserts and then aborts. A nested publish is supported and not reported. The check defaults to on in debug builds (`SUB0PUB_ASSERT` without `NDEBUG`) and off in release builds; it costs a `thread_local` frame per publish. Define `SUB0PUB_REENTRANT_CHECK true` to keep it in release. It has no effect with snapshot dispatch.

**Action:** A debug build that hits the abort relies on unsupported behaviour. Enable `SUB0PUB_REENTRANT_SAFE` (or `sub0::Snapshot` for the type) or restructure the subscriber.

### Configuration macros for per-type configuration (new in v2)

- `SUB0PUB_CONFIG_HEADER`: a header, named by the build system, that may define the project default configuration as `SUB0PUB_DEFAULT_CONFIG`.
- `SUB0PUB_DEFAULT_CONFIG`: the project default configuration type, e.g. `struct ProjectDefaults : sub0::with<sub0::Builtin, sub0::Capacity<4>> {};`.
- `SUB0PUB_CONFIGURE(Type, options...)`: configure a type you cannot modify.
- `SUB0PUB_CHECK_CONFIG`: the debug check for a type configured differently in two translation units (default: on without `NDEBUG`).
- `SUB0PUB_CONFIG_MISMATCH(what)`: the action when that check fails (default: assert, then abort).
- `SUB0PUB_DOMAIN_LIFETIME(what)`: the action when a `Domain` is destroyed while handles are still bound to it (default: assert, then abort).
- `SUB0PUB_NO_RECEIVERS_CHECK`, `SUB0PUB_NO_RECEIVERS(what)`: the check for a publication that reaches no receiver, and its action (since v2.0; default: on without `NDEBUG`; assert, then abort).
- `SUB0PUB_UNLISTED_CHECK`, `SUB0PUB_UNLISTED_RECEIVER(what)`: the check for a subscriber that its type's `StaticTo` list does not name, and its action (since v2.0; default: on without `NDEBUG`; assert, then abort).
- `SUB0PUB_AUDIT`, `SUB0PUB_AUDIT_PRINT(line)`, `SUB0PUB_AUDIT_EXIT(findings)`: the audit build, where its report goes and what follows it at exit (since v2.0; default: off; stderr; nothing).
- `SUB0PUB_EMPTY_BASES` (internal): the packed empty-base layout on the MSVC ABI.
- `SUB0PUB_TRACE` and `SUB0PUB_STRINGIFY` no longer exist (since v2.0, above).

The `SUB0PUB_*` policy macros must agree in every translation unit that uses a type. Setting them differently in one translation unit is only valid for types local to it.

### `SUB0PUB_FORCE_INLINE` (new in v2, internal)

Defined by `sub0pub/config_macros.hpp` for the static-wiring delivery chain: `__forceinline` on MSVC, plain `inline`
on every other compiler. It is not a configuration option (it has no `#ifndef` override). It is listed here only so
a project defining a macro of the same name sees the collision.

### `SUB0_STRINGIFY` removed

The library never used it, and v2 does not provide a replacement.

**Action:** Define your own if you used it: `#define STRINGIFY_(x) #x` and `#define STRINGIFY(x) STRINGIFY_(x)`.

---

## IPC Improvements

### BinaryReader now validates the prefix magic

In v1, the SUB0 magic prefix was read but never checked. In v2, the prefix is validated via `memcmp` against the expected value. A mismatch enters `SyncLost` state.

### Unknown typeIds are skipped instead of crashing

In v1, an unrecognized typeId in the stream caused a throw/assert with no recovery. In v2, the payload bytes (+ postfix) are discarded and the reader continues to the next frame. This allows version-skewed peers to coexist.

### SyncLost has a recovery path

In v1, `SyncLost` was a permanent dead end. In v2, the reader byte-scans forward for the next valid prefix magic and re-enters normal reading.
When exceptions are enabled, a corrupt prefix or postfix throws after entering `SyncLost`; catch the error and call
`update()` again to scan for the next valid frame.

### Optional postfix and writer polling

`BinaryReader<Prefix, Header, void>` now compiles when `update()` is used. The default `StreamSerializer` also accepts
`update()`: its synchronous binary writer has no pending work, so the call returns `true`.

### `paddingSize` type widened

Changed from `int_least16_t` to `int32_t` to prevent overflow on large payloads.

---

## IPC Design Notes

### Endianness

Sub0Pub does **not** perform per-message byte-swapping, and it is not a planned feature. All peers on a given IPC channel must share the same byte order; ensuring that, or converting where a platform needs it, is the application's responsibility. This is by design -- runtime endianness conversion would contradict the library's zero-overhead principle. For mixed-architecture deployments, a connection-time layout verification handshake is planned for a future phase (it detects a mismatch; it does not convert), with full type introspection via [Sub0Reflect](https://github.com/CraigHutchinson/Sub0Reflect).

### Type Layout Verification

The serialization protocol transmits raw bytes (`reinterpret_cast` of the data struct). Both peers must have identical struct layout. v2 provides `makeLayout<T>()` for connection-time verification:

```cpp
// Automatic -- no macros, no member lists
auto layout = sub0::utility::makeLayout<MyStruct>();
// layout.fingerprint: sizeof + alignof + arity + array info
// layout.layoutHash: per-member offset+size hash (GCC/Clang; 0 on MSVC)

// Compare layouts between peers at connection time
if (localLayout != remoteLayout) { /* reject connection */ }
```

`memberCount<T>` (and so the fingerprint arity) counts an array member as one member. Earlier v2 builds counted each array element separately (`struct { float d[4]; int t; }` reported 5, not 2). As a result, fingerprints of structs that contain arrays differ from those builds, and GCC now compiles `makeLayout<T>()` for such structs.

On GCC/Clang, the layout hash captures per-member offset and size via structured bindings (Boost.PFR-style), recursively fingerprinting nested structs and arrays. On MSVC, only the fingerprint (sizeof+alignof+arity) is available due to a compiler bug with structured bindings in template specializations. Full MSVC support is planned for C++26 reflection.

---

## Performance: v1.0 compared with v2

Measured with `python3 tests/compare/compare_versions.py`. It builds the same scenarios against the v1.0 tag's
header and the v2 header: each macro policy, selected per-type configurations and the static wiring, with
hand-written code as the floor. Full tables, both compilers and method notes are in
[docs/perf/compare-v1-v2-2026-09.md](docs/perf/compare-v1-v2-2026-09.md). The metric is exact instructions per
operation under callgrind, the repository's regression bar, not wall-clock time.

### Runtime (instr/op, gcc 13 -O2; clang 18 in brackets)

| Implementation | publish, 1 subscriber | publish, 8 subscribers | 8, first cancels | create + destroy |
|---|---:|---:|---:|---:|
| **v1.0** | 60 (60) | 221 (214) | 60 (57) | 48 (31) |
| v2 default | 34 (31) | 104 (101) | n/a (opt-in) | 47 (35) |
| v2 default, debug build checks | 60 (59) | 144 (150) | n/a (opt-in) | 91 (79) |
| v2 Full (`SUB0PUB_REENTRANT_SAFE`, `SUB0PUB_CANCEL`, `SUB0PUB_FILTER`) | 80 (73) | 290 (262) | 102 (91) | 54 (41) |
| v1.0 ThreadSafe | 130 (133) | 291 (287) | 130 (130) | 204 (192) |
| v2 ThreadSafe (with `SUB0PUB_FILTER`) | 260 (266) | 554 (516) | 301 (281) | 293 (290) |
| v2 static wiring (`StaticWiring`, `wire()`) | 8–9 (7–9) | 37 (37–40) | 15–16 (8–10) | n/a |
| hand-written direct calls | 9 (7) | 37 (37) | 15 (8) | n/a |

### Embedded footprint (Cortex-M33, one publisher, one subscriber, one publish site)

| Implementation | text / data / bss (bytes) | Needs thread-local storage | Other link-time dependencies |
|---|---|---|---|
| **v1.0** | 418 / 4 / 76 | yes | `operator delete` |
| v2 default | 224 / 4 / 58 | no | `memmove`, `__cxa_pure_virtual` |
| v2 Full | 390 / 4 / 62 | yes | `memcpy`, `memmove`, `__cxa_pure_virtual` |
| v2 `StaticWiring` | 12 / 0 / 4 | no | none |

### What this means when migrating

- **The default costs less than v1.0.** It publishes to 1 and 8 subscribers in 34 and 104 instructions, against
  v1.0's 60 and 221. Creating and destroying a subscriber costs 47 against 48 on gcc (35 against 31 on clang). The
  embedded image is 224 bytes against 418, and needs neither thread-local storage nor `operator delete`.
- **What you give up by default, you get back by opting in.** The default has no snapshot, no `cancel()`, no
  `filter()` and no lock. v1.0 had cancel and filter always on. Code that needs one of them is told:
  - `filter()` or `cancel()` without the opt-in does not compile;
  - subscribing or unsubscribing a type from its own `receive()`, or using a type from two threads at once, is
    reported in a debug build.
- **Opting in costs what the feature costs.** Full mode is snapshot dispatch, `cancel()` and `filter()`, v1.0's
  feature set plus the lifetime fixes below. It publishes to 1 and 8 subscribers in 80 and 290 instructions (v1.0: 60
  and 221), and it lets a subscriber unsubscribe or destroy itself, or another subscriber of its type, during a
  delivery without a use-after-free. v1.0 had no such guarantee.
- **Debug builds pay for detection.** Their checks cost a frame per publish and an atomic per operation, which is
  about 26 to 40 instructions per publish on gcc. Release builds (`NDEBUG`) have none.
- **`SUB0PUB_THREAD_SAFE` costs about twice v1.0's.** v1.0 is 130 and 291 instructions for 1 and 8 subscribers; v2 is
  260 and 554.
  - It buys teardown that is safe during concurrent delivery. v1.0's ThreadSafe mode could call a subscriber after it
    was destroyed.
  - A lighter lock than `std::mutex`, through `LockWith<L>`, closes most of the gap: a spin lock measures 134 and 421.
  - `SUB0PUB_THREAD_SAFE` still does not build on `arm-none-eabi`, which has no `std::mutex`. Use `LockWith<L>` with
    the RTOS lock.
- **Where the receivers are known when the application is composed, static wiring costs exactly what hand-written
  calls cost.** That is about 6 times fewer instructions than v1.0 for 8 subscribers, and 12 bytes of code.
