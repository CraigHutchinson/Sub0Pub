# Collapse evidence

Sub0Pub's claim is **correctness without cost**: wherever the application's topology allows it, the compiler removes
the library's dispatch, subscription, storage and context machinery. This document defines how that claim is
measured, and records the results for the public API. [DESIGN.md](DESIGN.md) records the decisions these results
support.

## Method

**Cases.** `tests/collapse/cases/<case>/` holds one application scenario per directory:

- `handwritten.cpp` is the equal-work reference, written without Sub0Pub.
- `handwritten_<kind>.cpp` are references for patterns that do more than direct calls, so each pattern is judged
  against what a careful engineer writes for the same job: `handwritten_runtime` (receiver addresses stored at setup
  and called through), `handwritten_erased` (a context pointer plus a function pointer), `handwritten_registry` (a
  hand-written dynamic registry), `handwritten_gateway` (one object holding two sessions' receivers),
  `handwritten_loop` (a loop over an array of receivers, pricing the unrolled fan-out). A variant
  selects one with a leading `// COLLAPSE_REFERENCE: handwritten_<kind>` line. Each extra reference is itself
  reported against `handwritten`, which prices the choice (runtime binding, type erasure, a registry) independently
  of any library.
- Every other file or directory implements the same behaviour through the public API (a *variant*). Case comments
  call the three explicit-wiring forms patterns B1, B2 and B3:

| Variant | Structure |
|---|---|
| `sub0_b1_*` (B1) | `wire(...)`: runtime addresses, static types |
| `sub0_b2_*` (B2) | `StaticWiring<&...>`: static storage |
| `sub0_b3_*` (B3) | `Sink<T>`: a type-erased port |
| `sub0_bridge_broker` | an explicit wiring with a `BrokerPort` for runtime subscribers |
| `sub0_typed_static` | `StaticTo` on the message type: the receivers and publisher of `sub0pub_virtual` (`Subscribe`, `Publish`, `publish()`), delivered by direct calls |
| `sub0_typed_first` | `StaticFirst` on the message type: the listed receivers by direct calls, then the runtime broker |
| `sub0_dynamic_*` | the runtime broker with a `Domain` or `Route` |
| `sub0pub_virtual`, `sub0pub_virtual_lean` | the runtime broker through `Subscribe`/`Publish`, default and leanest macros |

Each variant defines `collapse_setup()`, `collapse_publish(v)` and `collapse_teardown()`
(`tests/collapse/collapse_case.hpp`); anything inside them may be inlined.

**Forms.** Every case is built twice: *observable work* (receivers change observable state, and the checksum must
equal the reference's) and *removable work* (receivers do no observable work, so ideal code removes the machinery
around them). Argument side effects are observable in both forms.

**Behaviour (ctest, every CI compiler).** `tests/collapse/CMakeLists.txt` builds the selected cases in both forms
and fails if a variant's output differs from its reference. Normal local builds, pull requests, and merge pushes
use the representative public-API cases in `tests/collapse/smoke_cases.txt`. A manually dispatched CI run, or a
local build with `-DSUB0PUB_COLLAPSE_PROFILE=full`, covers all 18 cases.

**Evidence (`tests/collapse/collapse_evidence.py`).** For every case, variant, form and build, it links a real
executable and judges it against its reference of the same build and form:

| Evidence | Source | Criterion |
|---|---|---|
| behaviour | the executable's checksum | identical |
| publish / setup / teardown instructions | callgrind client requests in `driver.cpp` | publish within +2; setup and teardown no more |
| publish path | disassembly of `collapse_publish` plus the functions it reaches by direct calls | static instructions within +2; no extra indirect calls |
| RAM | `.data` + `.bss` | no more |
| static initialisation | `.init_array` | no more |
| retained Sub0Pub code | `sub0::` symbols left in the final image | none, or only if the image is no larger (the same code under another name, as a `Sink` thunk) |
| link dependencies | TLS, `operator delete`, `__cxa_pure_virtual`, atexit | none added |

Builds: `gcc-O2` and `clang-O2` (x86-64, run natively and under callgrind), `cm33-gcc-Os` (arm-none-eabi GCC,
Cortex-M33, final image only), each with an LTO counterpart for the multi-file case, and `msvc-O2` / `msvc-O2-lto`
on Windows. MSVC is measured from the final PE image: the publish path from `dumpbin /disasm`, sections and symbols
from the linker map (`cl /O2 /GS- /Gy /Gw`, linked with `/OPT:REF /OPT:NOICF`); a function taken from a library or
an import is external, like a PLT stub. Windows has no callgrind, so MSVC has no instruction counts: it is judged on
the publish path, image, RAM, retained code and dependencies, and on behaviour.

**Regression gate.** With `--budgets tests/collapse/budgets.json`, as CI runs it, every public-API variant must stay
within its recorded budget for each metric's delta against its reference, per build and form. A breach, a missing
budget or a missing measurement fails the run, as do behaviour mismatches, build or run failures and missing
references. Budgets are the measured deltas, never tighter than the criteria's tolerances; a deliberate change
re-records them with `--write-budgets` in the same commit. The cost criteria against hand-written code are reported,
not enforced: the runtime broker is expected to fail them, which is the price of runtime subscription.

Routine CI measures the representative cases with GCC, Clang, and Cortex-M33 in separate jobs. Each checks ordinary
and cross-file LTO builds against the recorded budgets. A manually dispatched CI run measures all cases and also
collects the full footprint and complete benchmark report, and runs all behaviour tests on MSVC. Every
selected case retains its variants, both forms, behaviour checks, and recorded budget gates. Separately, routine
CI checks selected runtime-broker and IPC instruction budgets in `tests/bench/budgets.json`.

## Cases

| Case | What it covers |
|---|---|
| `zero_receivers`, `one_receiver`, `multi_receivers`, `many_receivers` | fan-out: none, one, several of repeated types, 32 of one type |
| `multi_types` | one wiring carrying two message types; a receiver handling both |
| `filters` | an always-true filter that must disappear, and a runtime filter that keeps its branch |
| `large_payload` | a 64-byte message |
| `nested_publish` | a receiver publishing on its own wiring (another type, and the same type once) |
| `cancellation`, `cancellation_filtered` | a receiver stopping the rest of a publication, alone and with filters |
| `two_domains` | two sessions of one message type; one publisher feeding both |
| `transport_endpoint`, `transport_two_links` | a transport endpoint with egress and ingress (split horizon); two links of one transport type |
| `dynamic_subscriptions` | runtime subscribe and unsubscribe |
| `static_dynamic_bridge`, `_churn`, `_empty` | static wiring plus runtime subscribers: populated, churning, empty |
| `cross_file` | receivers in another translation unit, with and without LTO |

## Results (public API)

"=" means identical to the equal-work reference on every criterion, in both forms. Deltas are publish instructions
per publication (GCC / Clang) and Cortex-M33 image text, against that reference. `wire` is compared with
hand-written runtime binding and `Sink` with a hand-written context pointer plus function pointer.

| Case | Static wiring (`StaticWiring` / `wire` / `Sink`) | Runtime broker (default configuration) |
|---|---|---|
| Zero receivers | = / = / = | = |
| One receiver | = / = / = ¹ | +11 / +23; +416 B |
| Multiple receivers, repeated types | = / = / = ¹ | +56 / +46; +484 B |
| Default and runtime filters | = / = / = ¹ | with `sub0::Filter`: +67.5 / +55.5; +544 B |
| Two independent domains | = / = / = ¹; one publisher over both: `StaticWiring` =, `wire` = except Clang observable +3 (K23) | `Domain`: +71 / +63; +2232 B |
| Transport endpoint (egress and ingress) | = / = / = ¹; two links of one transport type: `StaticWiring` =, `wire` +8 / +10 unless each link has its own type (K18) | `Route`: +116 / +113; +716 B and TLS |
| Dynamic subscriptions | not applicable: runtime subscribers are the broker's (`StaticFirst`, `BrokerPort`) | against a hand-written registry with the same features: +3.5 / -1.5; +32 B |
| Cross-file, LTO off / on | = / = / = ¹ | +25 / +24 without LTO, +56 / +44 with LTO: LTO does not devirtualise the registry |

¹ `Sink` on Clang fails only the static publish-path criterion: Clang inlines the type-erased call (no indirect call
remains), and the metric then counts the inlined receiver as path instructions.

**A type's own topology.** `StaticTo` on the message type (`sub0_typed_static`: the receivers and publisher of the
runtime-broker variant, with one line added to the type) is identical to the equal-work reference on every criterion,
in both forms, on GCC, Clang and Cortex-M33, with and without LTO: 42 of 42 case × form × build checks over
`zero_receivers`, `one_receiver`, `many_receivers`, `multi_types`, `filters` and `cross_file`. On MSVC it equals the
`StaticWiring` variant: identical to hand-written code in five of the cases, and +2 path instructions at 32 receivers
(the application's own `send()`, as below). Adding the option to a type costs the runtime broker nothing: the brokered
`Subscribe` and `Publish` use the delivery policy of their message type; their runtime costs are covered by the broker budgets.

`StaticFirst` (`sub0_typed_first`) is judged against the hand-written registry, as the `BrokerPort` bridge is. Deltas
against that reference, observable form (publish instructions GCC / Clang; x86-64 RAM GCC / Clang; Cortex-M33 text
and RAM):

| Runtime side | `StaticFirst` on the type | `StaticWiring` with a `BrokerPort` and a `Domain` |
|---|---|---|
| One subscriber | +27 / -1; +192 / +208 B; +184 B and +20 B | +0 / -1; +112 / +104 B; +1332 B and +116 B |
| Never populated | +17 / +0; +200 / +208 B; +512 B and +16 B | +1 / +0; +16 / +24 B; +1224 B and +108 B |

The two are not the same work: the bridge variant carries a `Domain`, which is most of its Cortex-M33 text, and its
fixed receivers are plain classes. The receivers a `StaticFirst` list names are `Subscribe<T>` classes (K30), which
costs their virtual tables and subscription state in RAM, and on GCC one thing more: with a single subscribing class
in the program GCC devirtualises the runtime dispatch (the hand-written registry and the bridge have no indirect call
left), and the listed receivers make that class one of three, so the call stays indirect.

Remaining gaps, each a known limitation in [DESIGN.md](DESIGN.md#known-limitations): nested publication on one
static wiring (K22), two links of one transport type (K18, K23), cancellation combined with `filter()` (K24), and
the `BrokerPort` bridge's setup, teardown and RAM (the price of its policy), and `StaticFirst`'s fixed receivers
being broker subscribers by type (K30).

Every static-wiring variant against its reference, case × form checks meeting every criterion (with and without
LTO):

| Build | `wire` (`sub0_b1_*`) | `StaticWiring` (`sub0_b2_*`) | `Sink` (`sub0_b3_*`) |
|---|---:|---:|---:|
| GCC 13 `-O2` | 31 / 34 | 32 / 34 | 22 / 22 |
| Clang 18 `-O2` | 29 / 34 | 33 / 34 | 12 / 22 ¹ |
| arm-none-eabi GCC 13 `-Os` (Cortex-M33) | 32 / 34 | 33 / 34 | 22 / 22 |
| MSVC 19.51 `/O2` (no instruction counts) | 30 / 34 | 29 / 34 | 22 / 22 |

**MSVC.** Every variant behaves identically to its reference. The failures are the cases GCC and Clang fail
(`transport_two_links` K18, `cancellation_filtered` K24), plus:

- `many_receivers`, `wire`: the 32-binding wiring keeps out-of-line Sub0Pub code (path equal, text +304 B); GCC
  collapses it fully.
- `many_receivers`, `StaticWiring`, observable: the application's own `send()` stays out of line (path +2, text
  +48 B, RAM +16 B of alignment); the removable form passes.
- `nested_publish`, `StaticWiring`, observable: RAM +16 B; LTCG `cross_file`, `StaticWiring`: RAM +8 B.

MSVC's `/O2` inliner leaves a long delivery chain out of line (32 receivers: publish path 169 against 69
hand-written). The wiring's delivery functions are therefore `__forceinline` on MSVC only (`SUB0PUB_FORCE_INLINE`),
which brings the path to 71 (+2); other compilers get plain `inline`, so their code is unchanged.

## Reproduce

```bash
python3 tests/collapse/collapse_evidence.py [--case one_receiver] [--build gcc-O2] [--build gcc-O2-lto] [--json out.json] > report.md
python3 tests/collapse/collapse_evidence.py --budgets tests/collapse/budgets.json    # the CI regression gate
python tests/collapse/collapse_evidence.py --build msvc-O2                           # Windows, any prompt
```

The GCC, Clang and Cortex-M33 builds need `g++`, `clang++`, `arm-none-eabi-g++` and valgrind; on Windows the tool
finds the newest Visual Studio itself. CI collects the GCC, Clang and Cortex-M33 reports and their budget gates
as artifacts. The MSVC job runs the behaviour tests; use the commands above to collect MSVC final-image evidence
locally. Review reports for the exact revision under consideration, with their compiler, flags and reference
variants; dated captures do not describe a later revision automatically.
