# Per-type configuration

The broker policy belongs to the message type. Every publisher, subscriber, and translation unit using a given type
must see the same configuration. Inconsistent definitions across translation units violate the program's ODR; the
debug check can report mismatches it observes, but is not a substitute for consistent build configuration.

## Set a type's configuration

The common form is a member alias on the message type:

```cpp
struct Reading {
    int value;
    using sub0_config = sub0::config<sub0::Capacity<16>, sub0::Filter>;
};
```

`sub0::config<Options...>` starts from the project default and applies options left to right. Prefer putting the
configuration next to the type so all consumers see the same policy.

For types you cannot modify, use `SUB0PUB_CONFIGURE` near the type declaration:

```cpp
SUB0PUB_CONFIGURE(int, sub0::Capacity<16>);
```

Other supported forms are an ADL declaration next to the type
(`sub0::config<...> sub0_config(gps::Fix*);`) and a `Tagged<Payload, Tag>` type whose tag carries a configuration.
Configure a type in only one place.

## Options

| Option | Effect | Choose it when / cost |
|---|---|---|
| `Capacity<N>` | Fixed subscription-table capacity for this type. | More than the inherited capacity of 8 subscribers may be active. Uses fixed storage; no heap allocation. Check `isSubscribed()` / `trySubscribe()` for capacity failure. |
| `Direct` | Iterates the live subscriber table. | Use the lowest-overhead broker dispatch when the table will not change during its active dispatch. Same-type subscribe/unsubscribe during `receive()` is unsupported. |
| `DirectChecked` | Direct dispatch with a checked-build re-entrancy diagnostic. | Detect unsupported table changes without snapshotting. Detection does not make the operation safe. |
| `Snapshot` | Copies the subscriber table before dispatch; selects `ThreadLocalContext` if no context is already selected. | Subscribers may add/remove/destroy same-type subscribers during delivery. Pays for a table copy and publish-frame storage. |
| `ThreadLocalContext` | Stores per-publication frame state in TLS. | Needed for `cancel()`, routes, publish reports, and snapshot dispatch. TLS alone does not make concurrent broker access safe. |
| `StaticContext` | Stores frame state in a static, non-TLS frame. | Context features are needed on a single-threaded target without TLS. |
| `NoContext` | Omits publish-frame state. | Use the smallest configuration when no context-dependent feature is required. Cannot be combined with `Snapshot`. |
| `Filter` / `NoFilter` | Enables or disables `Subscribe<T>::filter()`. | Enable only when per-subscriber predicates are needed; filtering costs a virtual call per subscriber per publication. |
| `LockWith<L>` | Uses a lock type providing `lock()` and `unlock()`; selects `Snapshot` and `ThreadLocalContext`. | Publishers or teardown can overlap across threads. Callback state still needs its own synchronization. Locked subscribers use explicit activation and teardown; see the [thread-safe example](../examples/thread_safe_lifetime.cpp). |
| `Scoped` | Stores the table in `Domain<T>` instances instead of one global table. | Independent sessions need separate subscriber sets. A domain must outlive its bound handles. |
| `Implementation<Broker>` | Replaces the library broker with an application-supplied broker template. | Advanced customization. Custom implementations currently support global storage only. |

Options apply left to right; later options override earlier values where applicable. Invalid combinations are compile
errors. In particular, snapshots require a context, a lock requires snapshot dispatch and thread-local context, and
`Domain<T>` requires `Scoped`.

## Project-wide defaults

Set `SUB0PUB_CONFIG_HEADER` in the build and have that header define `SUB0PUB_DEFAULT_CONFIG`. For example:

```cpp
struct ProjectDefaults : sub0::with<sub0::Builtin, sub0::NoFilter> {};
#define SUB0PUB_DEFAULT_CONFIG ProjectDefaults
```

Every translation unit using the configured types must include the same project-default header. Per-type
`sub0::config<...>` options are applied on top of the project default.

## Configuration macros

Define macros before the first Sub0Pub header is included. Policy macros determine the default for message types
without an explicit configuration; diagnostics and serialization macros configure other library behavior.

| Macro | Default | Purpose |
|---|---|---|
| `SUB0PUB_MAX_SUBSCRIPTIONS` | `8` | Default fixed broker capacity per message type. |
| `SUB0PUB_REENTRANT_SAFE` | `false` | Snapshot dispatch by default; supports same-type table changes during `receive()`. |
| `SUB0PUB_CANCEL` | `false` | Enables context-dependent `cancel()`, routes, and publish reports by default. |
| `SUB0PUB_FILTER` | `false` | Enables subscriber `filter()` by default. |
| `SUB0PUB_THREAD_SAFE` | `false` | Uses a mutex and snapshot dispatch for the default policy. |
| `SUB0PUB_REENTRANT_CHECK` | Debug: `true`; `NDEBUG`: `false` | Detects same-type table changes during direct dispatch; not a safety mechanism. |
| `SUB0PUB_THREAD_CHECK` | Debug: `true`; `NDEBUG`: `false` | Detects some overlapping unlocked cross-thread use; not synchronization. |
| `SUB0PUB_CHECK_CONFIG` | Debug: `true`; `NDEBUG`: `false` | Reports observed per-type configuration mismatches across translation units. |
| `SUB0PUB_REENTRANT_VIOLATION(what)` | Assert, then abort | Handler for a detected direct-dispatch re-entrancy violation. If it returns, the unsupported operation continues. |
| `SUB0PUB_THREAD_VIOLATION(what)` | Assert, then abort | Handler for a detected unlocked cross-thread overlap. |
| `SUB0PUB_CONFIG_MISMATCH(what)` | Assert, then abort | Handler for a detected configuration mismatch. |
| `SUB0PUB_DOMAIN_LIFETIME(what)` | Assert, then abort | Handler when a domain is destroyed while handles remain bound. |
| `SUB0PUB_TRACE` | `false` | Enables event trace logging to `std::cout`. |
| `SUB0PUB_ASSERT` | `true` | Enables assertion-based checks. |
| `SUB0PUB_STD` | `false` | Selects standard streams instead of Sub0Pub's lightweight stream types. |
| `SUB0PUB_TYPEIDNAME` | `false` | Enables user-defined type IDs and names for diagnostics and IPC. |

Policy macros that affect a type's configuration must agree in every translation unit that uses that type. Prefer
`SUB0PUB_CONFIG_HEADER` or the per-type forms above over inconsistent translation-unit-local definitions.

See [design contracts](DESIGN.md#per-type-configuration-of-the-runtime-broker) for the configuration resolution
order and [migration notes](../MIGRATION.md) for v1 behavior changes.
