# Sub0Pub compile-time A/B

- UTC: 2026-09-30T14:44:48.685628+00:00
- Compiler: `g++ (Ubuntu 13.3.0-6ubuntu2~24.04) 13.3.0`
- Host: `Linux-6.18.44-x86_64-with-glibc2.39`; CPU: INTEL(R) XEON(R) PLATINUM 8573C; logical CPUs: 9
- A: `cf0fd36561589ccf19d7f90b2d16a55d44901146`, `c++23`
- B: `17047154ae9e2cdd8233b3378ea0c883b3d0cd56`, `c++23`
- 8 TUs/batch, 8 message types/TU, 4 receivers; 5 paired samples, 1 warmup batches/lane/profile, serial compilation.
- Flags: `-O2 -DNDEBUG -pthread`; no PCH, modules, compiler cache, configure or link.
- Warm filesystem cache; each sample recompiles every TU to fresh object files.

| Workload | A median [min, max] s | B median [min, max] s | B vs A | A / B MAD s |
|---|---:|---:|---:|---:|
| wiring | 2.122 [1.828, 3.213] | 1.836 [1.776, 3.183] | -13.48% | 0.294 / 0.060 |
| broker | 13.235 [10.975, 15.678] | 10.095 [9.898, 12.548] | -23.72% | 2.006 / 0.197 |
| umbrella | 6.373 [6.272, 6.597] | 6.364 [6.282, 6.695] | -0.14% | 0.088 / 0.062 |
| layout | 1.684 [1.643, 1.971] | 0.911 [0.857, 1.145] | -45.92% | 0.041 / 0.025 |

Positive change means slower. Samples alternate A/B then B/A; raw paired timings and harness/fixture
hashes are in the JSON. Ranges and MAD describe observed noise, not confidence intervals.
Synthetic serial clean-compile cost is not parallel application build time or incremental/no-op time.
Compare revisions within this run; do not compare absolute seconds across different hosts.
