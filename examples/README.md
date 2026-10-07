# Examples

Each source starts with a short guide: **Use when**, **Demonstrates**, **Story**, **Keep in mind**, and **Run**.
Find your case below, then read the source on its own. The tables follow the order of preference in the
[usage guide](../docs/USAGE.md): begin with the runtime broker, let a message type name its receivers once they are
known, and reach for explicit wiring only in the cases listed under it.

```sh
cmake --preset default
cmake --build --preset default
ctest --preset default -R Sub0Pub_Example_
```

## 1. Publish and subscribe (start here)

| Need | Source |
|---|---|
| The smallest publisher and subscriber | [minimal_sub0pub](minimal_sub0pub/main.cpp) |
| Receivers join and leave through object lifetime | [basic_pubsub](basic_pubsub/main.cpp) |
| One publisher sends several message types | [multi_type](multi_type/main.cpp) |
| Different receivers accept different message values | [filtering](filtering/main.cpp) |
| A primary handler claims a command before a fallback | [cancellation](cancellation/main.cpp) |
| A bounded session with a one-shot recorder and a waiting recorder | [dynamic_lifetime.cpp](dynamic_lifetime.cpp) |
| Two publishing threads share a receiver | [thread_safe_lifetime.cpp](thread_safe_lifetime.cpp) |

## 2. The receivers are known: let the message type name them

| Need | Source |
|---|---|
| One application built twice from one source: runtime subscription, then direct calls | [promote_to_static](promote_to_static/main.cpp) |
| A message type wired to one fixed receiver | [static_addresses.cpp](static_addresses.cpp) |
| A fixed controller with diagnostic probes that come and go beside it | [dynamic_diagnostics.cpp](dynamic_diagnostics.cpp) |

Not sure whether a type's receivers are fixed, or why a message is not arriving? Let a run tell you:

| Need | Source |
|---|---|
| Find publications nobody received, receivers never called, and types ready for `StaticTo` | [audit_findings.cpp](audit_findings.cpp) |

## 3. Explicit wiring: the cases a message type cannot express

| Need | Source |
|---|---|
| Receivers are local objects | [local_wiring.cpp](local_wiring.cpp) |
| A receiver stops the rest of a publication on the direct-call path | [static_cancellation.cpp](static_cancellation.cpp) |
| A publisher that cannot name its wiring (a library, a non-template interface) | [sink_output.cpp](sink_output.cpp) |
| Runtime probes that disconnect during delivery, in their own session, behind a wiring | [scoped_diagnostics.cpp](scoped_diagnostics.cpp) |

## 4. Transports and IPC

| Need | Source |
|---|---|
| Forward over a link without immediately echoing incoming messages | [link_forwarding.cpp](link_forwarding.cpp) |
| Forward with transport and receiver addresses fixed in the wiring type | [static_link_forwarding.cpp](static_link_forwarding.cpp) |
| Report transport rejection while preserving local delivery | [route_reports.cpp](route_reports.cpp) |
| Serialize typed messages and replay them from a byte buffer | [ipc_pipe](ipc_pipe/main.cpp) |
| Inspect message layouts before exchanging raw bytes | [layout_check](layout_check/main.cpp) |
| Investigate a shared-library / DLL boundary (currently disabled) | [cross_module](cross_module/main.cpp), [status](cross_module/README.md) |

The focused recipes (including `audit_findings`) and both `promote_to_static` builds return a failure code when their checks fail, including
with `NDEBUG`. The minimal example also checks its result; the other enabled introductory examples print their story.
These are teaching examples, not performance evidence; cross-module (DLL) use is not supported yet.
See [the design](../docs/DESIGN.md) for the decisions behind the tiers, and [migration](../MIGRATION.md) for v1 users.
New examples follow the [sample-header convention](../STYLE_GUIDE.md#examples-a-source-first-reading-guide).
