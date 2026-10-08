# Per-type configuration

How a message type is delivered belongs to the message type: its broker policy, and whether it is brokered at all. Every publisher, subscriber, and translation unit using a given type
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

`sub0::config<Options...>` starts from the project default and applies options left to right. Configure a type in
only one place, and put that place where nobody can see the type without it. In order of preference:

| Form | Use it when | Seen by every user of the type? |
|---|---|---|
| A member alias: `using sub0_config = sub0::config<...>;` | You own the type. This is the normal form. | Yes: it is part of the type. |
| A declaration beside the type, found by argument-dependent lookup: `sub0::config<...> sub0_config(gps::Fix*);` | You own the type's header but the type cannot take a member (an enumeration, a generated or C struct). | Yes: it is in the type's header and namespace. |
| `sub0::Tagged<Payload, Tag>`, whose tag carries the member alias | The payload is a builtin or a shared type, or one payload needs a second, differently configured message type. | Yes: the tag is part of the message type. |
| `SUB0PUB_CONFIGURE(Type, options...)` | You cannot change the type's header at all (a third-party type). | **No.** See below. |

`SUB0PUB_CONFIGURE` specialises a trait, and nothing ties that specialisation to the type: every translation unit
that uses the type must include the header containing it before its first use. A unit that misses it silently
configures the type with the project default, which is an ODR violation. A debug build reports what it observes at
run time (`SUB0PUB_CHECK_CONFIG`; for a type the macro wires with `StaticTo`, the stray unit's publication is also
reported as reaching nobody), but that is not a substitute. Use it only for a type you cannot modify, in one header
that every user of that type includes, and prefer wrapping the payload in a `Tagged` type of your own instead.

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
| `StaticTo<&a, &b>` | Delivers the type by direct calls to the listed receivers, in that order. `Subscribe<T>` and `Publish<T>` become empty: no table, no registration, no virtual call. | The type's receivers are a closed set of objects with static storage. Nothing that needs a table is available for the type (`disconnect()`, `cancel()`, `Domain`, `Route`), a subscriber the list does not name is never called, and a publisher's translation unit needs the receivers' definitions. See [usage](USAGE.md). |
| `StaticFirst<&a>` | Calls the listed receivers directly, then publishes through the broker to runtime subscribers. | Some receivers of the type are fixed and others come and go. The listed receivers are not registered and do not use `Capacity`; the broker's cost remains on every publication. |
| `AllowNoReceivers` | A publication of this type that reaches nobody is not reported. | An absent receiver is expected: a diagnostic stream, a plug-in loaded at run time, a type published under test. `sub0::receiverCount<T>(publisher)` lets a call site decide for itself. |
| `ReportNoReceivers` | A publication of this type that reaches nobody is reported in every build. | A message that must be heard, where the debug-only default is not enough. |
| `Scoped` | Stores the table in `Domain<T>` instances instead of one global table. | Independent sessions need separate subscriber sets. A domain must outlive its bound handles. |
| `Implementation<Broker>` | Replaces the library broker with an application-supplied broker template. | Advanced customization. Custom implementations currently support global storage only. |

Options apply left to right; later options override earlier values where applicable. Invalid combinations are compile
errors. In particular, snapshots require a context, a lock requires snapshot dispatch and thread-local context, and
`Domain<T>` requires `Scoped`. A `StaticTo` list in which nobody can receive the type is a compile error unless the
type is also `AllowNoReceivers`; a listed receiver's `filter()` needs the type's `Filter`.

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
| `SUB0PUB_CHECK_CONFIG` | Debug: `true`; `NDEBUG`: `false` | Reports observed per-type configuration mismatches across translation units, including a type one unit wires statically and another does not. |
| `SUB0PUB_NO_RECEIVERS_CHECK` | Debug: `true`; `NDEBUG`: `false` | Reports a publication that reaches no receiver, for every type that is not `AllowNoReceivers`. Define `true` to check release builds too, `false` to never report. |
| `SUB0PUB_UNLISTED_CHECK` | Debug: `true`; `NDEBUG`: `false` | Reports the construction of a subscriber that its type's `StaticTo` list does not name. |
| `SUB0PUB_REENTRANT_VIOLATION(what)` | Assert, then abort | Handler for a detected direct-dispatch re-entrancy violation. If it returns, the unsupported operation continues. |
| `SUB0PUB_THREAD_VIOLATION(what)` | Assert, then abort | Handler for a detected unlocked cross-thread overlap. |
| `SUB0PUB_CONFIG_MISMATCH(what)` | Assert, then abort | Handler for a detected configuration mismatch. |
| `SUB0PUB_DOMAIN_LIFETIME(what)` | Assert, then abort | Handler when a domain is destroyed while handles remain bound. |
| `SUB0PUB_NO_RECEIVERS(what)` | Assert, then abort | Handler for a publication that reached no receiver. If it returns, the publication is dropped. |
| `SUB0PUB_UNLISTED_RECEIVER(what)` | Assert, then abort | Handler for a subscriber its type's `StaticTo` list does not name. If it returns, the subscriber exists but is never called. |
| `SUB0PUB_TRACE` | `false` | Enables event trace logging to `std::cout`. |
| `SUB0PUB_ASSERT` | `true` | Enables assertion-based checks. |
| `SUB0PUB_STD` | `false` | Selects standard streams instead of Sub0Pub's lightweight stream types. |
| `SUB0PUB_TYPEIDNAME` | `false` | Enables user-defined type IDs and names for diagnostics and IPC. |

Policy macros that affect a type's configuration must agree in every translation unit that uses that type. Prefer
`SUB0PUB_CONFIG_HEADER` or the per-type forms above over inconsistent translation-unit-local definitions.

See [design contracts](DESIGN.md#per-type-configuration-of-the-runtime-broker) for the configuration resolution
order and [migration notes](../MIGRATION.md) for v1 behavior changes.
