# Sub0Pub Project Rules

## Build & Test

```bash
cmake --preset default        # Configure
cmake --build --preset default  # Build
ctest --preset default          # Run tests
```

Benchmarks are built on demand and are not run by ctest:
```bash
cmake --build --preset default --target Sub0Pub_Bench Sub0Pub_Bench_Checked Sub0Pub_Bench_Full Sub0Pub_Bench_ThreadSafe Sub0Pub_Bench_Ipc Sub0Pub_Bench_Axes
./build/tests/Release/Sub0Pub_Bench   # Windows
./build/tests/Sub0Pub_Bench           # Linux/macOS
python3 tests/bench/run_baseline.py   # all policies + IPC, with callgrind instr/op (Linux)
python3 tests/footprint/measure_footprint.py  # code size / RAM, host + Cortex-M33
python3 tests/compare/compare_versions.py     # v1.0 vs v2 (MIGRATION.md evidence)
python3 tests/collapse/collapse_evidence.py --budgets tests/collapse/budgets.json  # final-link gate (docs/EVIDENCE.md)
```
Compare against `docs/PERFORMANCE_BASELINE.md`: instr/op is the regression bar; ns/op is noisy.

Compile time is also a performance metric for header-only consumers. Changes to public headers, constraints,
configuration machinery or language baseline that may materially affect build cost should capture A/B evidence
with `tests/compile_time/compare.py` (see `docs/COMPILE_TIME.md`). Compare identical consumer workloads/compiler,
resolve exact revisions, retain raw repeated samples, and distinguish language migration from same-mode source
changes. Timing is advisory on shared hosts; investigate meaningful regressions rather than widening runtime
budgets or imposing an arbitrary noisy wall-time gate. Do not claim parallel/incremental build results from
serial clean-object measurements.

## Commit Rules

### Migration Document
Any commit that changes the public API surface in `include/sub0pub/` (the umbrella `sub0pub.hpp` and the headers it includes) MUST update `MIGRATION.md` with the breaking change details. The public API includes:
- `sub0::Subscribe`, `sub0::Publish`, `sub0::SubscribeAll`
- `sub0::ForwardSubscribe`, `sub0::ForwardPublish`, `sub0::ForwardSubscribeAll`, `sub0::ForwardPublishAll`
- `sub0::StreamSerializer`, `sub0::StreamDeserializer`
- Free functions: `sub0::publish()`, `sub0::cancel()`, `sub0::receiverCount()`
- Configuration macros: `SUB0PUB_*`
- `sub0::IPublish`, `sub0::Buffer`, `sub0::DefaultSerialisation`
- Per-type configuration: `sub0::config`, `sub0::config_t`, `sub0::configure`, `sub0::with`, `sub0::Builtin`, the options (`Capacity`, `Snapshot`, `Direct`, `DirectChecked`, `ThreadLocalContext`, `StaticContext`, `NoContext`, `LockWith`, `NoFilter`, `Scoped`, `Implementation`, `StaticTo`, `StaticFirst`, `AllowNoReceivers`, `ReportNoReceivers`), `sub0::Domain`, `sub0::Route`, `sub0::Tagged`, `sub0::SubscribeResult`, `sub0::SendResult`, `sub0::PublishReport`, `sub0::kit`
- Explicit wiring: `sub0::wire`, `sub0::Wiring`, `sub0::StaticWiring`, `sub0::Sink`, `sub0::Forward`, `sub0::StaticForward`, `sub0::BrokerPort`, `sub0::handles_v`

### Style
Follow `STYLE_GUIDE.md` for all C++ code. Key points:
- 4 spaces, no tabs
- `noexcept` on all publish/receive hot-path functions
- `SUB0PUB_` prefix for all configuration macros

### Examples
Every C++ sample and companion file under `examples/` must follow the source-first header convention in
`STYLE_GUIDE.md` (Use when, Demonstrates, Story, Keep in mind, Run). Review headers against the code and
observable output, including existing and disabled examples. Use a source-only first-reader review for
readability; explanatory sample comments are encouraged where they help a developer choose or adapt a pattern.

Examples and guides present one API in tiers of preference: the runtime broker by default, `StaticTo` /
`StaticFirst` on the message type when its receivers are known, explicit wiring only for the cases a type cannot
express (`docs/USAGE.md`). A new way to do something an existing tier already does needs a real-world case of its
own; otherwise it replaces the old way or is not added.

Examples teach the current API without version-based tiers. Isolate actual backwards-compatibility
adapters and their samples as removable migration debt; do not classify current runtime broker APIs as
legacy merely because their names existed in v1. See `STYLE_GUIDE.md` for placement and removal criteria.

### Tests
- All new features must have corresponding tests in `tests/`
- Performance-sensitive changes should be validated with `Sub0Pub_Bench` / `run_baseline.py` against `docs/PERFORMANCE_BASELINE.md`
- Tests must pass locally before committing: `ctest --preset default`
- A git pre-push hook runs tests automatically — set up with: `git config core.hooksPath .githooks`

## Design and evidence
- `docs/DESIGN.md` records the design, its decisions and its known limitations (K-numbers); `docs/EVIDENCE.md` how
  collapse is measured. Keep both current with the code: a changed decision or limitation updates them in the same commit.
- Design research (prototypes, face-offs, dated reports) is preserved at the tag `v2-research-archive`. New exploratory
  work lives on its own branch; the release tree carries only the public API, its tests, evidence and documentation.

## Branch Strategy
- `main` — current v2 release
- `develop` — ongoing v2 integration
- `v2` — v2 release-staging history
- `v1.0` tag — final v1 state
