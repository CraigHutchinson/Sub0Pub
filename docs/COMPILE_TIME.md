# Compile-time performance

Build time is a separate performance dimension from publish latency, code size and RAM. Sub0Pub is header-only:
include parsing and template instantiation repeat across consumer translation units. Changes to public headers,
constraints, configuration machinery or the language baseline should include compile-time A/B evidence when they
can materially affect those costs.

## Run a comparison

From a checkout that has both commits, with Python 3 and a direct GCC or Clang executable:

```sh
python3 tests/compile_time/compare.py --baseline <base-ref> --candidate <head-ref> \
  --translation-units 8 --json ab.json --markdown ab.md
```

Refs are resolved to full commit IDs, and only the headers archived at those commits are measured: uncommitted
library edits are **not**. The harness and fixture come from the invoking checkout, and their SHA-256 hashes are
recorded; both lanes use the same fixture and compiler. `--baseline-standard` and `--candidate-standard` (default
`c++23`) select each lane's language mode, for example `c++17` to reproduce the migration capture below.

## Workloads

| Profile | Includes | Instantiated work |
|---|---|---|
| wiring | `sub0pub/wiring.hpp` | Multiple message types and receivers, fan-out, Sink construction/copy/publication |
| broker | `sub0pub/broker.hpp` | Per-message subscriptions, virtual receive, publisher and subscription/teardown |
| umbrella | `sub0pub/sub0pub.hpp` | Exactly the wiring work, through the full include surface |
| layout | `sub0pub/utility/layout.hpp` | Distinct 32-member aggregates and recursive array fingerprints |

The umbrella/wiring pair helps expose the cost of the full include surface, but the principal comparison is
**A versus B within each profile**. Broker and wiring do different work and their absolute timings are not a
dispatch-performance ranking. Each TU uses distinct message types, representing independent consumer components.
The layout profile varies with `--types`; `--receivers` does not affect it.
This is a synthetic consumer compile workload; it does not time this repository's test suite.

The default is 16 translation units per batch, eight message types per TU, four receivers, five recorded paired
samples and one warmup batch per lane/profile. The committed capture and CI use eight TUs to limit run time.
`--translation-units`, `--types` and `--receivers` vary repeated parsing and template breadth independently;
receiver counts are limited to the broker's default capacity of eight. For a larger local study, use 16/64 TUs
and retain each report separately. Do not simply extrapolate these serial times into parallel application builds.

## What the timer includes

- Every TU is compiled serially with `-O2 -DNDEBUG -pthread`, the selected standard, and identical workload defines.
- Compilation starts in a fresh compiler process and produces a fresh object; timing includes process startup,
  preprocessing, parsing, template instantiation, optimization and object emission.
- Snapshot extraction, source generation, object cleanup, warmups, configure and link are outside the timer.
- Filesystem caches are warm after warmups. This is a **clean object compilation**, not a cold machine/disk cache test.
- No PCH, modules, unity builds, ccache or sccache. Invoke the real compiler; arbitrary custom wrappers are unsupported.
- No parallel compiler scheduling: this isolates aggregate compiler work from build-system parallelism and memory contention.

The tool creates temporary snapshots and never edits either source revision. Failed compiler processes or missing
object outputs abort the run rather than becoming a fast timing. The quick harness contract tests are in CTest;
the expensive timed benchmark is opt-in and has its own CI workflow.

## Interpretation and review

Samples alternate A/B then B/A, including warmups, to reduce order/thermal drift. JSON retains every measured pair
and its order, compiler/version, CPU/platform/affinity, include environment, flags, exact source revisions and
fixture/harness hashes. Reports show median, min/max, median absolute deviation (MAD), and percentage change
of B's median relative to A. Positive change means slower. No outliers are silently removed.

Ranges and MAD are descriptive, not confidence intervals. Small deltas within observed variation are inconclusive.
Use a quiet machine, rerun suspicious changes, and compare within the same run. Shared CI hosts add noise; do not
treat a small timing delta as a hard failure or compare absolute seconds across hosts/compiler versions.

The `Compile time A/B` workflow captures the same-language comparison on pull requests to `main` and also the migration comparison
when the base declares C++17. It uploads raw JSON and Markdown and adds a job summary. Compilation/harness failures
fail the job, but there is deliberately no arbitrary wall-clock percentage gate. Meaningful regressions should
be investigated and justified alongside the existing runtime/footprint gates.

These results do not measure MSVC, parallel build latency, incremental/no-op rebuilds, linker cost, peak compiler
memory or application-specific include graphs. Those are distinct studies if a consumer's build profile warrants them.

## Recorded results (September 2026)

**Moving from C++17 to C++23 costs compile time.** Clean serial compilation of eight consumer translation units, the
v2 headers in C++17 against the same headers moved to C++23 in C++23 mode
([report](perf/compile-time/cxx23-migration.md), [raw samples](perf/compile-time/cxx23-migration.json)):
wiring +58.7%, broker +61.8%, umbrella +82.8%. The ranges do not overlap, so this is a real cost for these workloads,
not a prediction for every application. Holding both lanes in C++23 separates the source changes from the mode
([report](perf/compile-time/cxx23-source.md), [raw samples](perf/compile-time/cxx23-source.json)): wiring -14.6%,
broker +0.9%, umbrella +16.0%, with overlapping ranges in every profile, so no source-level effect is established.
The two captures ran on different hosts: do not subtract them or compare their absolute seconds.

**The compile-cost reductions** (arity detection, the broker include surface; both lanes C++23, same host;
[report](perf/compile-time/optimization.md), [raw samples](perf/compile-time/optimization.json)): layout -45.9% with
non-overlapping ranges; broker -23.7%, whose ranges overlap; umbrella -0.1%; wiring -13.5%, whose headers and workload
did not change, so it is noise. They reduce, and do not cancel, the migration cost.

**The converged API (October 2026).** `StaticTo`, `StaticFirst` and the no-receivers policy, with the `Publisher` mixin
and `DynamicPort` removed, against the headers before them (both lanes C++23, same host, Clang 22 on Windows;
[report](perf/compile-time/api-convergence.md), [raw samples](perf/compile-time/api-convergence.json)): wiring -0.5%,
broker +1.3%, umbrella -0.0%, layout +0.8%, with overlapping ranges in every profile, so no effect is established. The
workloads do not use the new options: this is what they cost code that does not ask for them.
