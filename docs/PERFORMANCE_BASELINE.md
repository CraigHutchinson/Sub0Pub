# Performance baseline

The bar that changes to Sub0Pub are measured against. **The zero-cost rule:** a change must not raise instruction
counts or byte counts for users who do not opt in to it. Routine CI gates representative static-wiring cases against
their recorded final-link budgets ([EVIDENCE.md](EVIDENCE.md)) and selected runtime-broker and IPC instruction counts
against [`tests/bench/budgets.json`](../tests/bench/budgets.json). Broader measurements remain available on demand.

## How it is measured

| Metric | Tool | Use |
|---|---|---|
| **instr/op** | `valgrind --tool=callgrind`, per-scenario dumps through client requests (`tests/bench/bench_harness.hpp`) | Deterministic for a given compiler and flags: **the regression bar** |
| ns/op | nanobench | Machine- and noise-dependent: ±10–20% run to run on shared machines |
| text/data/bss, symbol sizes, link dependencies | `size`, `nm -S` on `-Os -fno-exceptions -fno-rtti` objects (`tests/footprint/`) | Embedded cost per usage pattern and per configuration option |

```bash
cmake --preset default
cmake --build --preset default --target Sub0Pub_Bench Sub0Pub_Bench_Checked Sub0Pub_Bench_Full Sub0Pub_Bench_ThreadSafe Sub0Pub_Bench_Ipc Sub0Pub_Bench_Axes
python3 tests/bench/run_baseline.py build/tests      # instr/op + ns/op: each policy, each configuration option, IPC
python3 tests/bench/run_baseline.py build/tests --no-timing --budgets tests/bench/budgets.json  # release gate
python3 tests/footprint/measure_footprint.py         # host + Cortex-M33 (arm-none-eabi-g++ if installed)
python3 tests/compare/compare_versions.py            # v1.0 against v2, same scenarios
```

**Control conditions.** Every benchmark scenario is the worst case for the runtime broker: publish entry points and
lifetime operations are out of line, and subscribers are defined out of line with more than one implementation, so
the optimiser cannot inline or devirtualise dispatch at the benchmark site. Collapse is measured separately, by the
evidence cases.

## Runtime broker (instr/op, GCC 13 `-O2`)

`tests/bench/run_baseline.py --no-timing`, measured on GCC 13.3.0 in [the full v2 CI run](https://github.com/CraigHutchinson/Sub0Pub/actions/runs/36790377271).
The selected rows in `tests/bench/budgets.json` are enforced on each PR and merge; the other rows provide context:

| Scenario | Default (Direct) | Default, debug-build check | Full (Snapshot, `cancel()`, `filter()`) | ThreadSafe (`std::mutex`, `filter()`) |
|---|---:|---:|---:|---:|
| publish, 0 subscribers | 24 | 37 | 67 | 236 |
| publish, 1 subscriber | 38 | 48 | 98 | 277 |
| publish, 8 subscribers | 101 | 125 | 293 | 557 |
| 8 subscribers, first cancels | n/a | n/a | 105 | 318 |
| re-entrant publish, depth 1 | 74 | 100 | 200 | 554 |
| create + destroy subscriber | 46 | 61 | 57 | 279 |
| unsubscribe first of 8 + resubscribe | 83 | 99 | 94 | 322 |
| `trySubscribe()`, table full | 12 | 16 | 12 | 92 |

What the opt-in features pay for: **Full** lets a subscriber be unsubscribed or destroyed during a dispatch, or from
inside `filter()`, without being called afterwards (limitations K1 and K2 in [DESIGN.md](DESIGN.md)).
**ThreadSafe** pays for teardown that is safe during concurrent delivery (K3); a lighter lock through `LockWith<L>`
costs less. `Sub0Pub_Bench_Axes` measures each configuration option alone.

Floors, without Sub0Pub:

| Floor | 1 receiver | 8 receivers |
|---|---:|---:|
| direct call, compiler may inline (what static wiring reaches) | 6 | 13 |
| virtual `receive()` loop | 11 | 89 |
| virtual `filter()` + `receive()` loop | 25 | 118 |
| `std::function` loop | 30 | 100 |

## Footprint (Cortex-M33, `-Os`)

One publisher, one subscriber, one publish site ([perf/compare-v1-v2-2026-09.md](perf/compare-v1-v2-2026-09.md)):

| Implementation | text / data / bss (bytes) | Thread-local storage | Other link-time dependencies |
|---|---|---|---|
| v2 default | 224 / 4 / 58 | no | `memmove`, `__cxa_pure_virtual` |
| v2 Full | 390 / 4 / 62 | yes | `memcpy`, `memmove`, `__cxa_pure_virtual` |
| v2 `StaticWiring` | 12 / 0 / 4 | no | none |
| v1.0 | 418 / 4 / 76 | yes | `operator delete` |

Each further message type instantiates its own table and dispatch loop (K17). `measure_footprint.py` reports the
cost of each configuration option from the same scenario, and the marginal cost of a second subscriber, publish site
and message type: [perf/footprint-2026-09.md](perf/footprint-2026-09.md).

## Not yet measured

- Contended locking (several threads publishing one type), teardown latency, and ISR-context publish.
- Embedded stack use: Snapshot dispatch copies the table to the stack, so size `Capacity` to the real bound.
- Cross-core or real-transport IPC, and receive-to-dispatch latency on target hardware.
- RISC-V: the Ubuntu `riscv64-unknown-elf` toolchain ships without libstdc++, so this needs a full toolchain (for
  example the Zephyr SDK).
