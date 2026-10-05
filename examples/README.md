# Examples

Each source starts with a short guide: **Use when**, **Demonstrates**, **Story**, **Keep in mind**, and **Run**.
Choose a use case below, then read the source on its own. Static wiring, the runtime broker,
mixed paths and IPC are all parts of the current API. Examples are organized by use case, not API generation.

```sh
cmake --preset default
cmake --build --preset default
ctest --preset default -R Sub0Pub_Example_
```

| Need | Source |
|---|---|
| The smallest runtime publisher and subscriber | [minimal_sub0pub](minimal_sub0pub/main.cpp) |
| Receivers join and leave through object lifetime | [basic_pubsub](basic_pubsub/main.cpp) |
| One publisher sends several message types | [multi_type](multi_type/main.cpp) |
| Different receivers accept different message values | [filtering](filtering/main.cpp) |
| A primary handler claims a command before a fallback | [cancellation](cancellation/main.cpp) |
| Wire known local receivers directly | [local_wiring.cpp](local_wiring.cpp) |
| Hide the receiver-list type behind a `Sink` | [sink_output.cpp](sink_output.cpp) |
| Encode static receiver addresses in the wiring type | [static_addresses.cpp](static_addresses.cpp) |
| Stop delivery in fixed wiring with a bool result | [static_cancellation.cpp](static_cancellation.cpp) |
| A bounded session with a one-shot recorder and a waiting recorder | [dynamic_lifetime.cpp](dynamic_lifetime.cpp) |
| Two publishing threads share a receiver | [thread_safe_lifetime.cpp](thread_safe_lifetime.cpp) |
| Explicitly add/remove a diagnostic probe beside a fixed controller | [dynamic_diagnostics.cpp](dynamic_diagnostics.cpp) |
| Diagnostic probes disconnect during delivery using broker policy | [scoped_diagnostics.cpp](scoped_diagnostics.cpp) |
| Forward over a link without immediately echoing incoming messages | [link_forwarding.cpp](link_forwarding.cpp) |
| Forward with transport and receiver addresses fixed in the wiring type | [static_link_forwarding.cpp](static_link_forwarding.cpp) |
| Report transport rejection while preserving local delivery | [route_reports.cpp](route_reports.cpp) |
| Serialize typed messages and replay them from a byte buffer | [ipc_pipe](ipc_pipe/main.cpp) |
| Inspect message layouts before exchanging raw bytes | [layout_check](layout_check/main.cpp) |
| Investigate a shared-library / DLL boundary (currently disabled) | [cross_module](cross_module/main.cpp), [status](cross_module/README.md) |

The eleven focused recipes return a failure code when their checks fail, including with `NDEBUG`.
The minimal example also checks its result; the other enabled introductory examples print their story.
These are teaching examples, not performance evidence; cross-module (DLL) use is not supported yet.
See [the design](../docs/DESIGN.md) for choosing between the structures, and [migration](../MIGRATION.md) for v1 users.
New examples follow the [sample-header convention](../STYLE_GUIDE.md#examples-a-source-first-reading-guide).
