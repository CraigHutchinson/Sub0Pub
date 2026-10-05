# Library comparisons and bridge candidates

[Back to the README](../README.md#how-it-compares)

Sub0Pub v2 offers two delivery models: a bounded runtime subscription table per message type (optionally
per `Domain`), and explicitly composed `wire()` / `StaticWiring` direct calls. Both deliver synchronously on
the publishing thread. Locking makes concurrent broker use safe; it does not schedule callbacks on another thread.
The broker and wiring do not allocate subscription storage from the heap; application callbacks, payloads,
streams and external adapters may allocate.

The closest alternatives depend on whether you need embedded message routing, typed events, or an event loop.
The links below are the upstream documentation (reviewed September 2026); these are architectural comparisons,
not head-to-head performance results.

| Library / facility | Model and strengths | Where Sub0Pub differs |
|---|---|---|
| **Sub0Pub v2** | C++23, header-only; typed messages, fixed-capacity subscriptions, optional scoped domains and static wiring. | Single-threaded subscribers register on construction; concurrent configurations require explicit activation. A subscriber disconnects in its destructor, and once `disconnect()` returns its `receive()` is not called again on any thread; a concurrent `disconnect()` may wait for one callback in progress. Static wiring binds objects at composition time (runtime receivers join through `DynamicPort` or `BrokerPort`) and does not track lifetimes: bound receivers must outlive the wiring. No built-in event queue or scheduler. |
| [ETL messaging](https://www.etlcpp.com/docs/messaging/) (`message_router`, `message_bus`, `message_broker`) | Close embedded peer: message IDs, typed router handlers, bounded buses and explicit subscriptions by message ID. Useful when an application already uses ETL's embedded utilities. | Sub0Pub's local routing uses the C++ message type without requiring an ETL message base or numeric ID; static wiring can call plain receivers directly. |
| [EnTT events and signals](https://github.com/skypjack/entt/wiki/Events,-signals-and-everything-in-between) | Close typed-event peer: delegates/signals and a dispatcher with immediate `trigger` or queued `enqueue`/`update`. Useful beyond its ECS. | EnTT also supports typed events and compile-time callback binding. Sub0Pub focuses on bounded broker storage and explicitly composed static fan-out. |
| [eventpp](https://github.com/wqking/eventpp) | Header-only callback lists, event-key dispatch and queues, with configurable policies and mixins. Useful for runtime listener composition and deferred processing. | Sub0Pub separates message types and static receiver composition; queued processing must be supplied by the application or an adapter. |
| [Zephyr zbus](https://docs.zephyrproject.org/latest/services/zbus/index.html) | Close RTOS peer: shared channels, synchronous listeners and asynchronous observer options integrated with Zephyr. | Sub0Pub is independent of an RTOS and passes messages through callbacks; it does not provide zbus's shared-channel state or observer queues. |
| [Boost.Signals2](https://www.boost.org/doc/libs/latest/doc/html/signals2.html) | Header-only signal/slot connections, result combiners and tracked object lifetimes, with thread-safety support. Useful for flexible callback graphs. | Sub0Pub prioritises bounded storage and static wiring. Its concurrent `disconnect()` contract should not be assumed for Signals2: a slot already executing may continue after disconnection ([thread safety](https://www.boost.org/doc/libs/latest/doc/html/signals2/thread-safety.html)). |
| [Qt signals/slots](https://doc.qt.io/qt-6/signalsandslots.html) | QObject connections, automatic disconnection on destruction, direct or queued delivery and event-loop integration. A natural fit for Qt applications. | Sub0Pub needs no Qt runtime or MOC-generated signal machinery. Broker subscribers also disconnect automatically on destruction, but static wiring does not track lifetimes, and there is no queued delivery or thread-affinity handling. |

Sub0Pub's stream serialization is an additional facility, not an IPC transport or a delivery guarantee.
Applications still provide the stream/transport and agree on the wire representation; adapters to other event
systems do not automatically make their payloads serializable. Allocation and performance comparisons need
matched workloads, connection lifetimes and queue policies; see our [measured design](../README.md#measured-design) for
what has actually been measured.

## Bridge candidates (proposed, not implemented)

A small optional adapter can connect an existing event system to Sub0Pub without replacing either one.
These are feasibility priorities, not supported integrations or scheduled commitments:

| Candidate | First useful bridge | Contract to resolve before shipping |
|---|---|---|
| **ETL — first prototype** | Explicit conversions between selected ETL messages and plain Sub0Pub payloads; an ETL router receives ingress and an egress adapter calls the target bus/router. | Map IDs, destination addressing and payload ownership explicitly; do not imply that ETL subscription behavior is identical to Sub0Pub's. |
| **EnTT — first prototype** | Selected dispatcher event types feed a Sub0Pub sink; egress chooses `trigger` for immediate delivery or `enqueue` for deferred delivery. | Own and disconnect listener connections; keep `update()` under application control and copy/own queued payloads. |
| **Zephyr zbus — embedded follow-up** | Map selected message types to channels; publish outbound values and copy inbound messages into a worker-owned queue before dispatching to Sub0Pub. | Choose channel-notification versus message-copy semantics, bound queue storage, report overflow/timeouts, and respect ISR rules. Avoid recursively publishing to a channel from its locked listener callback. |
| **Qt — application follow-up** | A QObject adapter converts selected signals into typed messages and posts outbound values to the receiver's thread. | Register queued payload types where required, own queued values, respect thread affinity and disconnect before destroying adapter state. |
| **Boost.Signals2 / eventpp — on demand** | A connection-owning callback adapter for selected signatures/event keys. | Define teardown during in-flight callbacks, exception handling and any queue-draining policy. Implement when a concrete consumer needs it. |

Reuse the existing boundaries: `Forward<Adapter>` for static egress, with ingress through
`wiring.publishFrom(adapter, msg)` (or `publishFrom<Adapter>(msg)`), which skips the adapter's own binding; or
`Route<T, Adapter>` for a runtime broker/domain with `send(const T&) noexcept -> SendResult`, whose `inject()` does
the same. `Sink<T>` suits one-way ingress only: it carries no origin, so an adapter that is also bound for egress
would receive its own messages back. `Route` requires a publish context; `Forward` does not collect send results.
Keep dependencies in optional adapter headers/targets. This split horizon covers synchronous two-way flow; a queued
hop loses the dispatch context, so an asynchronous two-way bridge needs its own origin policy to prevent feedback.
Never retain a borrowed callback payload across an asynchronous boundary, let an exception escape a `noexcept`
callback, or equate queue acceptance with delivery.
A prototype should prove payload conversion, ordering, teardown, overflow reporting and loop prevention, then
measure allocations, instructions and code size against hand-written glue before an API is committed.
