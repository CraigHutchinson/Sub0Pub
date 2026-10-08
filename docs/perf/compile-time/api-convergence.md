# Sub0Pub compile-time A/B

- UTC: 2026-10-07T13:26:05.675811+00:00
- Compiler: `clang version 22.1.8 (https://github.com/llvm/llvm-project ca7933e47d3a3451d81e72ac174dcb5aa28b59d1)`
- Host: `Windows-11-10.0.26220-SP0`; CPU: Intel64 Family 6 Model 198 Stepping 2, GenuineIntel; logical CPUs: 24
- A: `04b75b51792f54b99548fe01caee6a1692114e96`, `c++23`
- B: `7a2c1e4755e5873a7b2c19fea27bbc30a2512f3d`, `c++23`
- 8 TUs/batch, 8 message types/TU, 4 receivers; 9 paired samples, 1 warmup batches/lane/profile, serial compilation.
- Flags: `-O2 -DNDEBUG -pthread`; no PCH, modules, compiler cache, configure or link.
- Warm filesystem cache; each sample recompiles every TU to fresh object files.

| Workload | A median [min, max] s | B median [min, max] s | B vs A | A / B MAD s |
|---|---:|---:|---:|---:|
| wiring | 1.714 [1.660, 1.768] | 1.705 [1.677, 1.759] | -0.50% | 0.019 / 0.008 |
| broker | 7.101 [6.479, 7.653] | 7.196 [6.665, 7.315] | +1.33% | 0.127 / 0.091 |
| umbrella | 4.707 [4.601, 4.769] | 4.705 [4.636, 4.735] | -0.04% | 0.042 / 0.022 |
| layout | 1.512 [1.478, 1.540] | 1.524 [1.496, 1.593] | +0.77% | 0.015 / 0.025 |

Positive change means slower. Samples alternate A/B then B/A; raw paired timings and harness/fixture
hashes are in the JSON. Ranges and MAD describe observed noise, not confidence intervals.
Synthetic serial clean-compile cost is not parallel application build time or incremental/no-op time.
Compare revisions within this run; do not compare absolute seconds across different hosts.
