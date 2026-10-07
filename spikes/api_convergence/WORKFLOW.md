# Evidence workflow for an API-convergence candidate

A candidate API is viable only if it changes what the user writes without changing what the machine does. This is
the sequence that establishes that, in the order to run it. Each stage has a tool, a criterion, and a recorded
result under [`results/`](results/). The method is the project's own ([docs/EVIDENCE.md](../../docs/EVIDENCE.md)):
every variant is a final-link executable judged against an equal-work reference.

Run everything from the repository root.

## Prerequisites

| Tool | Used for | Found by |
|---|---|---|
| Python 3, CMake | every stage | `PATH` |
| MSVC (`cl`, `link`, `dumpbin`) | builds, final-image analysis, `/Qvec-report` | newest Visual Studio, located with `vswhere` |
| clang (`clang++`, `clang-cl`) | second compiler; optimisation records for vectorisation and aliasing | `PATH`, else `C:\Program Files\LLVM\bin` |
| Intel `icx-cl` (optional) | third compiler; `/Qopt-report` | oneAPI `compiler\latest\bin` |
| Intel VTune Profiler (optional) | stage 6 | `PATH`, else oneAPI `vtune\latest\bin64` |
| GCC, valgrind (Linux) | callgrind instruction counts, the stock gate | `PATH` |

The tools locate the optional ones themselves ([`tools/spike_common.py`](tools/spike_common.py)) and leave out what
is missing; a stage whose required tool is missing says so and exits non-zero.

On Windows there is no callgrind. The spike driver ([`harness/driver.cpp`](harness/driver.cpp)) therefore has an
`--instr` mode that single-steps each phase (trap flag plus a vectored exception handler) and counts the instructions
executed: exact and repeatable, over the same region callgrind measures. Elsewhere the same driver issues the stock
callgrind client requests for the stock tool to read; that path is written but has not been run (see
[Not covered](#not-covered-on-this-machine)).

## Stages

### 1. Behaviour: the examples build and agree

```bash
cmake -S spikes/api_convergence -B spikes/api_convergence/build
cmake --build spikes/api_convergence/build --config Release
ctest --test-dir spikes/api_convergence/build -C Release
```

Every candidate is one weather station ([`examples/`](examples/)) built in several delivery modes from shared
sources. Each executable runs the same flow and returns non-zero if a receiver missed a message. Build `Debug` as
well: the runtime broker's re-entrancy and thread checks, and the typed route's "subscriber not listed in its
`StaticTo`" assertion, are debug-build checks.

Three of the typed route's builds are repeated as audit builds (`convergence_typed_route_<mode>_audit`), which
record the run and print, at exit, each type's publishers and receivers and any finding
([`include/sub0pub_spike/audit.hpp`](include/sub0pub_spike/audit.hpp)). `convergence_audit_findings` is a station
with deliberate mistakes whose test checks the audit's count.

**Criterion:** every test passes, in Release and Debug, on each compiler, with no warnings at `/W4` or
`-Wall -Wextra -Wpedantic`; the station's audit builds report no findings.

### 2. Client cost: what the user edits to switch mode

```bash
python spikes/api_convergence/tools/client_diff.py
```

Counts the lines of code that differ between a candidate's `mode/dynamic/` and each other `mode/<mode>/`. Everything
outside `mode/` is compiled unchanged by every build.

**Criterion:** reported, not gated. The aim is a small difference that stays out of the publisher and receiver
classes ("participant lines edited" = 0).

### 3. A/B shootout: instructions, publish path, image and RAM

```bash
python spikes/api_convergence/tools/shootout.py [--case station] [--build msvc-O2] [--timing] [--keep DIR] [--json out.json] > report.md
```

The stock collapse-evidence tool, pointed at [`cases/`](cases/). A variant names its reference in its first lines:

| Variant | Judged against | Must show |
|---|---|---|
| a candidate's static build (`*_static`) | `handwritten`: direct calls | every criterion of docs/EVIDENCE.md met, as `today_static` meets them |
| a candidate's dynamic build (`*_dynamic`) | `today_dynamic`: the current `Subscribe` / `Publish` | no metric above the current API |
| a type-erased build (`port_static`) | `handwritten_erased` | no more than type erasure itself costs |
| `today_static`, `today_dynamic` | `handwritten` | the baseline: what the current APIs cost |

Per variant and build it reports behaviour (checksum), instructions per publication, setup and teardown, the static
publish path and its indirect calls, image text, RAM, retained `sub0::` code and added link dependencies, in both the
observable-work and removable-work forms, then a cross-build summary. `--timing` adds wall-clock ns per publication,
which is noisy and supplemental.

**Criterion:** exit status 0 (no behaviour mismatch, no build or measurement failure); a gated static build is `=`
against hand-written code and a gated dynamic build is `=` against the current API on every build. A candidate that
is deliberately not equal states its price in the report.

### 4. Vectorisation and aliasing

```bash
python spikes/api_convergence/tools/vectorize_report.py [--case batch]
```

Instruction counts on single publications cannot show whether a collapsed delivery leaves the *caller's* loop
optimisable. The `batch` case publishes a burst of 64 samples into an accumulating receiver. The tool asks each
compiler what it did with that loop, with the default instruction set and with AVX2:

- clang optimisation records: was the loop vectorised, at what width; and how many loads on the publish path could
  not be hoisted or eliminated because a store may alias them (`licm` and `gvn` missed-load remarks);
- MSVC `/Qvec-report:2`: `C5001`, or `C5002` with its reason (1200 is a loop-carried dependence, which is how
  unresolved aliasing appears);
- icx `/Qopt-report`: loop and SLP vectorisation remarks.

**Criterion:** a candidate's static and dynamic builds are vectorised wherever their reference is, and clang reports
no more alias-blocked loads than for the reference. Other variants are reported against their reference.

### 5. Compile time (advisory)

A convergence layer is template machinery in a header-only library, so it has a build cost. Once a candidate moves
into `include/sub0pub/`, capture A/B evidence with `tests/compile_time/compare.py` as
[docs/COMPILE_TIME.md](../../docs/COMPILE_TIME.md) describes. Not measured for the spike layer, which no consumer
includes.

### 6. Profile: where the time goes

```bash
python spikes/api_convergence/tools/shootout.py --case station --build msvc-O2 --keep DIR > /dev/null
python spikes/api_convergence/tools/vtune_ab.py --keep DIR --build msvc-O2 --case station > vtune.md
```

VTune's hotspots collection on the kept executables, each publishing in a loop for a few seconds. Reported per
variant: ns per publication under the profiler, CPU time in the image, the share of it in `sub0::` functions, and the
functions holding most of it.

**Criterion:** a static build has no `sub0::` share (nothing of the library is left to sample); a dynamic build's
profile matches the current API's.

User-mode sampling (`--mode sw`, the default) needs no privileges. Hardware event-based sampling (`--mode hw`) needs
the VTune sampling driver and an elevated prompt; it is the route to `vtune -collect uarch-exploration`, which
attributes indirect-branch mispredictions and front-end stalls: the costs of dynamic dispatch that an instruction
count does not weigh. Run it when a dynamic-path change is proposed.

## Reading a result

- **Instruction counts are the comparison of record**; wall-clock time is ±10–20% run to run.
- Counts include the driver's loop and the call into the publication, identically for every variant, so only
  differences between variants mean anything.
- Counts differ between compilers for the same source. Compare a variant with its reference *within* a build.
- A static build that is `=` on one publication can still differ on a burst; stage 4 is what covers that.

## Not covered on this machine

- **GCC, callgrind and Cortex-M33.** The stock gate's Linux builds need `g++`, `clang++`, `arm-none-eabi-g++` and
  valgrind. `shootout.py` is written to run there (the driver issues callgrind client requests instead of
  single-stepping) but has only been run on Windows, and the spike driver has no bare-metal mode yet.
- **Budgets.** `shootout.py` passes `--budgets` / `--write-budgets` through to the stock tool; no budgets are
  recorded for the spike cases, because they are not part of the release gate.
- **Hardware-event profiling**, as above.
- **llvm-mca.** Not wired in: every gated static publish path was instruction-identical to its reference, so there
  was nothing for a throughput model to distinguish. It is the next tool to add if a candidate's static path ever
  differs by a few instructions and the question becomes what they cost in cycles.
