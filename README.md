# Sub0Pub

> Sub0Pub was originally written by hand in 2018 as a spare-time project exploring type-safe messaging in C++. In the current age of AI, we are leveraging it to accelerate development -- driving the concept to a more mature, production-ready level far faster than solo effort allows. This v2 release is **not yet field-tested**, but it has a full test suite, CI pipeline, and performance benchmarks, and will be integrated into future products. The `v1.0` tag preserves the final v1 baseline.

**Zero-overhead, type-safe, auto-wiring publish-subscribe for C++**

A header-only messaging library that uses C++ template specialization to route signals at compile time. No `connect()` calls, no signal objects, no MOC toolchain, no allocations. Just inherit, publish, and receive.

Built for **embedded systems**, **game loops**, **desktop applications**, and **messaging across process boundaries** (with an application-supplied transport).

```cpp
#include "sub0pub/sub0pub.hpp"

// Publish: inherit and call publish()
class Sensor : public sub0::Publish<float> {
public:
    void sample() { sub0::publish(this, 3.14f); }
};

// Subscribe: inherit and implement receive()
class Display : public sub0::Subscribe<float> {
    void receive(const float& value) noexcept override {
        // Automatically called when any Publish<float> fires
    }
};

int main() {
    Sensor sensor;
    Display display;   // Auto-wired on construction
    sensor.sample();   // display.receive(3.14f) called
}
```

---

## Why Sub0Pub?

### What It Does Well

- **Zero-friction wiring** -- Publishers and subscribers connect automatically on construction via the MonoState Broker pattern. No registry, no `connect()`, no boilerplate.
- **Compile-time type routing** -- Message dispatch is resolved entirely through template specialization. No `std::any`, no `void*`, no `dynamic_cast`, no runtime type lookups.
- **Zero allocation** -- No `shared_ptr`, no heap allocation in the hot path. Fixed-size subscription tables live in static storage.
- **Header-only** -- `#include <sub0pub/sub0pub.hpp>` for everything, or only the part you use (`sub0pub/broker.hpp`, `sub0pub/wiring.hpp`, `sub0pub/ipc.hpp`); link with `Sub0Pub::Sub0Pub` via CMake.
- **Multi-type subscription** -- `SubscribeAll<A, B, C>` or `SubscribeAll<std::tuple<A, B>>` to subscribe to many types in one class.
- **Built-in IPC serialization** -- `StreamSerializer` / `StreamDeserializer` with a composable binary protocol (`BinaryWriter<Prefix, Header, Postfix>`) that frames messages onto a stream you supply; the transport, byte order and delivery guarantees are the application's.
- **Pay only for what you use** -- The default is the cheapest dispatch: a loop of virtual calls. Snapshot dispatch, `cancel()`, `filter()` and locking are opt-in, and using one without opting in is caught: at compile time, or by a debug-build check.
- **Publish cancellation** -- Opt-in: subscribers call `cancel()` from within `receive()` to halt further delivery on the current publish cycle.
- **Message filtering** -- Opt-in: a `filter(const Data&)` override for per-subscriber message selection.

### How It Compares

**Sub0Pub is built for typed messaging with predictable storage and a small integration footprint.**
Its C++23, header-only core combines fixed-capacity subscriptions with static wiring that can reduce delivery
to direct calls. Filtering, cancellation, snapshots and locking are opt-in per message type.

| Compared with | Why choose Sub0Pub? | Trade-off |
|---|---|---|
| ETL messaging | Route plain C++ payloads locally without message base classes or numeric IDs; bind plain receivers with static wiring. | ETL provides a broader embedded toolkit and addressed message routing. |
| EnTT / eventpp | Combine bounded subscription storage and explicit static fan-out in a focused messaging library. | Queues and deferred processing need application support. |
| Boost.Signals2 / Qt | Fixed-capacity broker storage and direct-call static wiring, with no Qt runtime or MOC requirement. | Connection lifetime and event-loop facilities differ; adapters must preserve them explicitly. |
| Zephyr zbus | Use the same typed messaging core on bare metal, an RTOS or desktop. | No built-in shared-channel state or RTOS observer queues. |

These are design trade-offs, not cross-library speed rankings. See [comparisons and proposed bridges](docs/COMPARISONS.md)
for sources, lifetime details and integration options.

### Current Status

**v2.0.0-alpha:** complete and tested, not yet field-tested. CI builds and tests every change on Linux (GCC, Clang),
macOS and Windows (MSVC), under AddressSanitizer, UndefinedBehaviorSanitizer and ThreadSanitizer, and checks the
final-link code of every public-API pattern against a recorded budget.

### Measured design

The runtime broker's per-type configuration and the static wiring are measured against hand-written code doing the
same work. [docs/DESIGN.md](docs/DESIGN.md) records the design, its decisions and its known limitations with their
measured cost; [docs/EVIDENCE.md](docs/EVIDENCE.md) records how the code is measured and the results. For v1 users,
[MIGRATION.md](MIGRATION.md#performance-v10-compared-with-v2) compares v1.0 with v2.

### Design Decisions

**Endianness: out of scope, by design.** Sub0Pub IPC does not convert byte order, and doing so is not a planned
feature. Messages cross the channel as their in-memory representation, so every peer on a channel must share the
same byte order and layout. Checking that the build and platforms in use are compatible is the application's
responsibility, as is any conversion a mixed-endian deployment needs (for example in a transport adapter). This keeps
the IPC path at little or no cost: conversion on every message would violate the library's core principle. A basic
byte-swizzle example may be added later (lowest priority).

**Automatic layout verification.** `makeLayout<T>()` produces a `TypeLayout` containing sizeof, alignof, arity, array extent info, and a per-member layout hash -- all automatically via C++17 structured bindings (Boost.PFR-style). No macros, no member lists. On MSVC, per-member decomposition is deferred to C++26 reflection; the fingerprint (sizeof+alignof+arity) still catches most layout mismatches. Full type-member introspection is planned via [Sub0Reflect](https://github.com/CraigHutchinson/Sub0Reflect).

### Known Remaining Limitations

| Issue | Severity | Plan |
|-------|----------|------|
| **Cross-module isolation** -- a broker's static state is per module (DLL / shared library) | Medium | Not supported yet: [examples/cross_module](examples/cross_module/README.md) records what support needs |
| **No CRC/checksum** -- only magic prefix + postfix for framing | Low | Add optional integrity check to protocol |
| **Type hash not stable across compilers** -- `typeHash<T>()` uses `__PRETTY_FUNCTION__`/`__FUNCSIG__` | Medium | Use `SUB0PUB_TYPEIDNAME` for cross-compiler IPC |
| **MSVC layout hash limited** -- structured binding bug prevents per-member decomposition | Low | Awaiting C++26 `std::meta::reflect` |
| **No byte-order conversion** -- peers must share byte order and layout (see Design Decisions) | By design | Application responsibility; a basic byte-swizzle example is possible future work (lowest priority) |

The runtime broker's and the static wiring's limitations are listed, with their measured cost, in
[docs/DESIGN.md](docs/DESIGN.md#known-limitations).

### Planned

**IPC hardening:** an optional CRC/checksum protocol layer, a connection-time layout verification handshake, and
[Sub0Reflect](https://github.com/CraigHutchinson/Sub0Reflect) integration for deeper type introspection.

---

## Performance

Benchmarks are built alongside the tests and capture system information automatically:

```bash
./build/tests/Release/Sub0Pub_Bench   # Windows
./build/tests/Sub0Pub_Bench           # Linux/macOS
```

`Sub0Pub_Bench` runs the default configuration; `Sub0Pub_Bench_Checked`, `_Full` and `_ThreadSafe` run the same
scenarios with the debug-build check, with snapshot dispatch, `cancel()` and `filter()`, and with a lock.
`Sub0Pub_Bench_Axes` changes one configuration option at a time, and `Sub0Pub_Bench_Ipc` measures serialize and
deserialize end to end. `python3 tests/bench/run_baseline.py` runs them all and adds deterministic instruction
counts (valgrind); `python3 tests/footprint/measure_footprint.py` reports code size and RAM for the host and
Cortex-M33. Current results: [docs/PERFORMANCE_BASELINE.md](docs/PERFORMANCE_BASELINE.md). Build time is measured
separately, by clean multi-translation-unit consumer compiles: [docs/COMPILE_TIME.md](docs/COMPILE_TIME.md).

Sub0Pub and [Sub0Pipeline](https://github.com/CraigHutchinson/Sub0Pipeline) share the C++23 baseline; the
integration boundary and where an adapter should live are in [docs/INTEGRATION.md](docs/INTEGRATION.md).

---

## Getting Started

### Requirements

- A compiler and standard library supporting the C++23 features used by Sub0Pub. CMake propagates
  `cxx_std_23` through `Sub0Pub::Sub0Pub`; direct header users must select C++23 mode themselves.
  Current CI exercises GCC, Clang, AppleClang and MSVC; this is not a claim of complete C++23 feature support.
  MSVC toolchains may select `/std:c++latest` for C++23 through CMake.
- CMake 3.21+

### Build & Test

```bash
cmake --preset default            # Configure
cmake --build --preset default    # Build
ctest --preset default            # Run tests
```

### Install (system-wide)

```bash
cmake --build --preset default --target install
```

### Use in Your Project

```cmake
find_package(Sub0Pub REQUIRED)
target_link_libraries(MyApp PRIVATE Sub0Pub::Sub0Pub)
```

Or add as a subdirectory:

```cmake
add_subdirectory(Sub0Pub)
target_link_libraries(MyApp PRIVATE Sub0Pub::Sub0Pub)
```

### Headers

`#include <sub0pub/sub0pub.hpp>` includes the whole library. A translation unit that uses one part can include only
that part; each area directory holds one header per responsibility:

| Include | Provides | Needs |
|---|---|---|
| `sub0pub/broker.hpp` | the runtime broker: `Subscribe`, `Publish`, `SubscribeAll`, `Domain`, `Route`, `publish()`, `cancel()`, per-type configuration | configuration |
| `sub0pub/wiring.hpp` | static wiring: `wire()`, `StaticWiring`, `Sink`, `Publisher`, `Forward`, `DynamicPort` | nothing else from Sub0Pub |
| `sub0pub/ipc.hpp` | IPC serialisation: `StreamSerializer`, `StreamDeserializer`, `DefaultSerialisation` | streams and type identity only |
| `sub0pub/config.hpp` | per-type configuration and its resolution (`config_t<T>`) | the configuration macros |
| `sub0pub/wiring/broker_port.hpp`, `sub0pub/ipc/forward.hpp` | the bridges: `BrokerPort`; `ForwardSubscribe`, `ForwardPublish` | both parts they connect |

`SUB0PUB_*` macros are read when `sub0pub/config_macros.hpp` is first included, whichever Sub0Pub header includes it:
define them on the compiler command line or before the first Sub0Pub include.

---

## Examples

### Minimal Publish/Subscribe

```cpp
class PubInt : public sub0::Publish<uint32_t> {
public:
    void doIt() { sub0::publish(this, 42U); }
};

class SubInt : public sub0::Subscribe<uint32_t> {
    void receive(const uint32_t& value) noexcept override { total += value; }
};
```

### Multi-Type Subscribe

```cpp
class Listener : public sub0::SubscribeAll<float, int, std::string> {
    void receive(const float& f) noexcept override { /* ... */ }
    void receive(const int& i) noexcept override { /* ... */ }
    void receive(const std::string& s) noexcept override { /* ... */ }
};
```

### Message Filtering and Cancellation

Both are opt-in, per type or for every type (`SUB0PUB_FILTER`, `SUB0PUB_CANCEL`). Without the opt-in, a subscriber that
declares `filter()` or calls `cancel()` does not compile, so neither is silently ignored.

```cpp
struct Reading { int value; using sub0_config = sub0::config<sub0::Filter>; };
class EvenOnly : public sub0::Subscribe<Reading> {
    void receive(const Reading& r) noexcept override { /* handle even values */ }
    bool filter(const Reading& r) noexcept override { return (r.value % 2) == 0; }
};

struct Command { int id; using sub0_config = sub0::config<sub0::ThreadLocalContext>; };
class Claim : public sub0::Subscribe<Command> {
    void receive(const Command&) noexcept override { cancel(); } // later subscribers are skipped
};
```

### Subscriber Capacity

Each `Data` type has a fixed subscription table of `SUB0PUB_MAX_SUBSCRIPTIONS` entries (no heap allocation). Constructing a subscriber never fails, but if the table is already full the subscriber is **not registered** and will not receive data. This is reported, not asserted, and behaves identically in debug and release (`NDEBUG`) builds -- the table is never written past its end.

```cpp
MySubscriber sub;
if (!sub.isSubscribed()) {
    // Table was full: handle it (log, raise SUB0PUB_MAX_SUBSCRIPTIONS, or retry later)
}

// Retry once another subscriber of the same type has been destroyed
if (sub.trySubscribe() == sub0::SubscribeResult::CapacityExceeded) { /* still full */ }
```

Destroying a subscriber frees its slot. Destroying one that was never registered is a no-op. With `SubscribeAll<A, B>`, check each base: `sub.sub0::Subscribe<A>::isSubscribed()`.

### Re-entrancy Policy

A nested publish of the same type from `receive()` is always supported. Subscribing or unsubscribing that type from its own `receive()` (including destroying the subscriber) needs snapshot dispatch:

| Configuration | Cost | Same-type subscribe/unsubscribe from `receive()` |
|---|---|---|
| default (`SUB0PUB_REENTRANT_SAFE false`), release | none | Not supported |
| default, debug build (`SUB0PUB_REENTRANT_CHECK`) | a `thread_local` frame per publish | Detected: `SUB0PUB_REENTRANT_VIOLATION` |
| `SUB0PUB_REENTRANT_SAFE true`, or `sub0::Snapshot` per type | a table copy and a frame per publish | Supported |

`SUB0PUB_THREAD_SAFE` always uses the snapshot. Without a lock, a debug build also reports a `Data` type used from two threads at once (`SUB0PUB_THREAD_CHECK`).

### Per-Type Configuration

Each `Data` type can choose its own policy; types that don't use the `SUB0PUB_*` macros below.

```cpp
// A message whose subscribers come and go from inside receive(): snapshot dispatch, and cancel()
struct Imu {
    float accel[3];
    using sub0_config = sub0::config<sub0::Snapshot, sub0::ThreadLocalContext, sub0::Capacity<2>>;
};

// A type you cannot modify: configure it next to its declaration
SUB0PUB_CONFIGURE(int, sub0::Capacity<16>);

// Independent sessions of the same type
struct Command { int id; using sub0_config = sub0::config<sub0::Scoped>; };
sub0::Domain<Command> sessionA, sessionB;
struct Handler : sub0::Subscribe<Command> {
    using Subscribe::Subscribe;                 // Handler h(sessionA);
    void receive(const Command&) noexcept override {}
};
```

Options: `Capacity<N>`; `Snapshot` (selects `ThreadLocalContext` unless a context is chosen), `Direct` or `DirectChecked`; `ThreadLocalContext`, `StaticContext` (no TLS) or
`NoContext`; `LockWith<L>` (concurrent publishers; implies `Snapshot` and `ThreadLocalContext`); `Filter` or `NoFilter`;
`Scoped`; `Implementation<Broker>`. Invalid
combinations do not compile. A project-wide default can be set with `SUB0PUB_CONFIG_HEADER`. With a lock, call
`trySubscribe()` at the end of the most-derived constructor and `disconnect()` at the start of its destructor.

### Static Wiring

Where the receivers are known when the application is composed, bind them directly. Receivers are plain classes with
a non-virtual `receive()`; each delivery is a direct call, as fast as writing the calls by hand.

```cpp
struct Controller { void receive(const Sample& s) noexcept; };
struct Logger     { void receive(const Sample& s) noexcept; bool receive(const Fault& f) noexcept; };

Controller controller;
Logger logger;

using Bus = sub0::StaticWiring<&controller, &logger>;  // static storage: no RAM
Bus::publish(Sample{1});                                // controller.receive(), then logger.receive()
Bus::publishCancelable(Fault{});                        // a bool receive() returning false stops delivery

auto bus = sub0::wire(controller, logger);              // runtime addresses, same direct calls
sub0::Sink<Sample> port(bus);                           // type-erased port for a non-template publisher
```

`DynamicPort<T, N>` and `BrokerPort<T>` bind runtime subscribers into a static wiring; `Forward<Transport>` binds a
transport, and `publishFrom(transport, msg)` keeps ingress from echoing back out.

### Cross-Module / IPC Serialization

```cpp
// Serialize published signals to a stream
class Recorder : public sub0::StreamSerializer<>
               , public sub0::ForwardSubscribe<float, Recorder>
               , public sub0::ForwardSubscribe<int, Recorder> {
public:
    Recorder(sub0::OStream& out) : sub0::StreamSerializer<>(out) {}
};

// Deserialize and re-publish from a stream
class Player : public sub0::StreamDeserializer<>
             , public sub0::ForwardPublish<float, Player>
             , public sub0::ForwardPublish<int, Player> {
public:
    Player(sub0::IStream& in) : sub0::StreamDeserializer<>(in) {}
};
```

---

## Configuration

Compile-time feature flags (define before including the header):

| Flag | Default | Description |
|------|---------|-------------|
| `SUB0PUB_TRACE` | `false` | Enable event trace logging to `std::cout` |
| `SUB0PUB_ASSERT` | `true` | Enable assertion checks |
| `SUB0PUB_STD` | `false` | Use `std::ostream`/`std::istream` instead of lightweight internal stream types |
| `SUB0PUB_TYPEIDNAME` | `false` | Enable user-defined type IDs and names for IPC |
| `SUB0PUB_THREAD_SAFE` | `false` | Mutex guard for multi-threaded pub/sub (snapshot dispatch; subscribers call `trySubscribe()` after construction) |
| `SUB0PUB_REENTRANT_SAFE` | `false` | Snapshot dispatch: subscribe or unsubscribe a type from inside its own `receive()`. Costs a table copy and a frame per publish |
| `SUB0PUB_CANCEL` | `false` | Publish context: `cancel()`, `Route` and publish reports. Costs a `thread_local` frame per publish |
| `SUB0PUB_FILTER` | `false` | `filter()`: a virtual call per subscriber per publish |
| `SUB0PUB_REENTRANT_CHECK` | debug: `true`, `NDEBUG`: `false` | Without snapshot dispatch, detect a `receive()` that subscribes or unsubscribes its own `Data` type, and call `SUB0PUB_REENTRANT_VIOLATION(what)` |
| `SUB0PUB_THREAD_CHECK` | debug: `true`, `NDEBUG`: `false` | Without a lock, detect a `Data` type used from two threads at once, and call `SUB0PUB_THREAD_VIOLATION(what)` |
| `SUB0PUB_REENTRANT_VIOLATION(what)` | `assert` then `std::abort()` | Handler for a detected re-entrancy violation. Override to log or count; if it returns, the call continues unguarded |
| `SUB0PUB_MAX_SUBSCRIPTIONS` | `8` | Fixed subscription table size per `Broker<T>`. Subscribers beyond this are rejected (see [Subscriber Capacity](#subscriber-capacity)) |
| `SUB0PUB_CONFIG_HEADER` | unset | Header (set by the build system) that may define the project default configuration as `SUB0PUB_DEFAULT_CONFIG` |
| `SUB0PUB_CHECK_CONFIG` | debug: `true`, `NDEBUG`: `false` | Report a `Data` type configured differently in two translation units through `SUB0PUB_CONFIG_MISMATCH(what)` |

The policy macros (`SUB0PUB_MAX_SUBSCRIPTIONS`, `SUB0PUB_REENTRANT_SAFE`, `SUB0PUB_CANCEL`, `SUB0PUB_FILTER`, `SUB0PUB_THREAD_SAFE`) are the default configuration of every `Data` type that does not choose its own ([Per-Type Configuration](#per-type-configuration)). They must agree in every translation unit that uses a type.

---

## License

[MIT License](LICENSE.md) -- Copyright (c) 2018 Craig Hutchinson
