# Sub0Pub benchmark report

```
== System Info ==
OS: Linux
Compiler: GCC 13.3.0
CPU: AMD EPYC 7763 64-Core Processor
Threads: 4
RAM: 15 GB
Build: Release
==
```

instr/op: callgrind instruction count / 10000 iterations (includes ~3 instructions of loop overhead).

## Core publish/subscribe by policy

### instr/op

| Scenario | Direct (default) | Direct + check | Full (snapshot, cancel, filter) | ThreadSafe |
|---|---:|---:|---:|---:|
| **Publish** | | | | |
| 0 subscribers | 24.0 | 37.0 | 28.0 | 232.0 |
| 1 subscriber | 38.0 | 48.0 | 77.0 | 273.0 |
| 2 subscribers | 47.0 | 59.0 | 118.0 | 313.0 |
| 4 subscribers | 65.0 | 81.0 | 175.0 | 393.0 |
| 8 subscribers (max default) | 101.0 | 125.0 | 287.0 | 553.0 |
| **Multi-type dispatch** | | | | |
| int publish (2-type publisher) | 38.0 | 48.0 | 77.0 | 273.0 |
| float publish (2-type publisher) | 38.0 | 48.0 | 72.0 | 252.0 |
| **Re-entrant publish** | | | | |
| 1 subscriber, nested depth 1 | 74.0 | 100.0 | 158.0 | 546.0 |
| **Subscription lifetime** | | | | |
| create + destroy subscriber (empty table) | 46.0 | 61.0 | 57.0 | 279.0 |
| create + destroy subscriber (7 others) | 73.0 | 88.0 | 84.0 | 307.0 |
| unsubscribe first of 8 + resubscribe | 83.0 | 99.0 | 94.0 | 322.0 |
| create + destroy subscriber (table full, rejected) | 25.0 | 30.0 | 25.0 | 298.0 |
| trySubscribe() (table full, rejected) | 12.0 | 16.0 | 12.0 | 92.0 |
| trySubscribe() (already subscribed) | 9.0 | 9.0 | 9.0 | 16.0 |
| **Floor (no Sub0Pub)** | | | | |
| direct call (collapse target), 1 receiver | 6.0 | 6.0 | 6.0 | 6.0 |
| direct call (collapse target), 8 receivers | 13.0 | 13.0 | 13.0 | 13.0 |
| virtual receive loop, 1 receiver | 11.0 | 11.0 | 11.0 | 11.0 |
| virtual receive loop, 8 receivers | 89.0 | 89.0 | 89.0 | 89.0 |
| virtual filter+receive loop, 1 receiver | 25.0 | 25.0 | 25.0 | 25.0 |
| virtual filter+receive loop, 8 receivers | 118.0 | 118.0 | 118.0 | 118.0 |
| std::function loop, 1 receiver | 30.0 | 30.0 | 30.0 | 30.0 |
| std::function loop, 8 receivers | 100.0 | 100.0 | 100.0 | 100.0 |
| **Filter and cancel** | | | | |
| 1 filtered subscriber (pass) | - | - | 79.0 | 275.0 |
| 1 filtered subscriber (reject) | - | - | 71.0 | 265.0 |
| 8 subscribers, first cancels | - | - | 99.0 | 314.0 |

## IPC end-to-end

### instr/op

| Scenario | Default |
|---|---:|
| **IPC 4B payload (17B framed)** | |
| serialize: publish -> stream | 105.0 |
| deserialize: stream -> subscriber | 444.0 |
| floor: memcpy framed message | 23.0 |
| **IPC 64B payload (77B framed)** | |
| serialize: publish -> stream | 111.0 |
| deserialize: stream -> subscriber | 441.0 |
| floor: memcpy framed message | 34.0 |
| **IPC 256B payload (269B framed)** | |
| serialize: publish -> stream | 134.0 |
| deserialize: stream -> subscriber | 459.0 |
| floor: memcpy framed message | 65.0 |

## Runtime broker configuration, one option at a time (docs/DESIGN.md)

### instr/op

| Scenario | Configuration axes |
|---|---:|
| **D Full (Snapshot, ThreadLocal, filter, no lock, Global, capacity 8)** | |
| publish, 0 subscribers | 31.0 |
| publish, 1 subscriber | 73.0 |
| publish, 8 subscribers | 220.0 |
| create + destroy subscriber | 52.0 |
| re-entrant publish (depth 1) | 152.0 |
| **D Dispatch=Direct** | |
| publish, 0 subscribers | 46.0 |
| publish, 1 subscriber | 61.0 |
| publish, 8 subscribers | 166.0 |
| create + destroy subscriber | 52.0 |
| re-entrant publish (depth 1) | 128.0 |
| **D Dispatch=DirectChecked** | |
| publish, 0 subscribers | 46.0 |
| publish, 1 subscriber | 61.0 |
| publish, 8 subscribers | 166.0 |
| create + destroy subscriber | 60.0 |
| **D Context=Static** | |
| publish, 0 subscribers | 31.0 |
| publish, 1 subscriber | 73.0 |
| publish, 8 subscribers | 220.0 |
| create + destroy subscriber | 52.0 |
| re-entrant publish (depth 1) | 152.0 |
| **D Filter=off** | |
| publish, 0 subscribers | 27.0 |
| publish, 1 subscriber | 63.0 |
| publish, 8 subscribers | 182.0 |
| create + destroy subscriber | 52.0 |
| re-entrant publish (depth 1) | 132.0 |
| **D Lock=spin** | |
| publish, 0 subscribers | 97.0 |
| publish, 1 subscriber | 133.0 |
| publish, 8 subscribers | 378.0 |
| create + destroy subscriber | 112.0 |
| re-entrant publish (depth 1) | 274.0 |
| **D Lock=std::mutex** | |
| publish, 0 subscribers | 223.0 |
| publish, 1 subscriber | 259.0 |
| publish, 8 subscribers | 504.0 |
| create + destroy subscriber | 432.0 |
| re-entrant publish (depth 1) | 526.0 |
| **D Storage=Scoped** | |
| publish, 0 subscribers | 37.0 |
| publish, 1 subscriber | 78.0 |
| publish, 8 subscribers | 225.0 |
| create + destroy subscriber | 60.0 |
| re-entrant publish (depth 1) | 160.0 |
| **D Capacity=64** | |
| publish, 0 subscribers | 34.0 |
| publish, 1 subscriber | 78.0 |
| publish, 8 subscribers | 225.0 |
| create + destroy subscriber | 52.0 |
| re-entrant publish (depth 1) | 162.0 |
| **D Implementation=SingleSubscriberBroker** | |
| publish, 0 subscribers | 35.0 |
| publish, 1 subscriber | 47.0 |
| create + destroy subscriber | 33.0 |
| re-entrant publish (depth 1) | 100.0 |
| **D Route=1 (NullTransport)** | |
| publish, 1 route | 81.0 |
| publish, 1 subscriber + 1 route | 113.0 |
| **L Lean (Direct, NoContext, NoFilter, no lock, Global, capacity 8)** | |
| publish, 0 subscribers | 9.0 |
| publish, 1 subscriber | 33.0 |
| publish, 8 subscribers | 96.0 |
| create + destroy subscriber | 44.0 |
| re-entrant publish (depth 1) | 72.0 |
| **L Dispatch=Snapshot (+StaticContext, required)** | |
| publish, 0 subscribers | 27.0 |
| publish, 1 subscriber | 64.0 |
| publish, 8 subscribers | 183.0 |
| create + destroy subscriber | 52.0 |
| re-entrant publish (depth 1) | 134.0 |
| **L Dispatch=DirectChecked (+StaticContext, required)** | |
| publish, 0 subscribers | 41.0 |
| publish, 1 subscriber | 52.0 |
| publish, 8 subscribers | 129.0 |
| create + destroy subscriber | 60.0 |
| **L Context=Static** | |
| publish, 0 subscribers | 41.0 |
| publish, 1 subscriber | 52.0 |
| publish, 8 subscribers | 129.0 |
| create + destroy subscriber | 52.0 |
| re-entrant publish (depth 1) | 110.0 |
| **L Context=ThreadLocal** | |
| publish, 0 subscribers | 41.0 |
| publish, 1 subscriber | 52.0 |
| publish, 8 subscribers | 129.0 |
| create + destroy subscriber | 52.0 |
| re-entrant publish (depth 1) | 110.0 |
| **L Filter=on** | |
| publish, 0 subscribers | 9.0 |
| publish, 1 subscriber | 33.0 |
| publish, 8 subscribers | 96.0 |
| create + destroy subscriber | 44.0 |
| re-entrant publish (depth 1) | 72.0 |
| **L Storage=Scoped** | |
| publish, 0 subscribers | 19.0 |
| publish, 1 subscriber | 27.0 |
| publish, 8 subscribers | 83.0 |
| create + destroy subscriber | 55.0 |
| re-entrant publish (depth 1) | 58.0 |
| **L Capacity=64** | |
| publish, 0 subscribers | 9.0 |
| publish, 1 subscriber | 33.0 |
| publish, 8 subscribers | 96.0 |
| create + destroy subscriber | 45.0 |
| re-entrant publish (depth 1) | 72.0 |

