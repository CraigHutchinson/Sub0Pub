# Sub0Pub compile-time A/B

- UTC: 2026-09-30T09:08:51.567423+00:00
- Compiler: `g++ (Ubuntu 13.3.0-6ubuntu2~24.04) 13.3.0`
- Host: `Linux-6.18.44-x86_64-with-glibc2.39`; CPU: AMD EPYC 9V74 80-Core Processor; logical CPUs: 9
- A: `ab3622277018b7d18cc507afad64af1d1bfbe6a6`, `c++17`
- B: `1e8f1dcef8ed29fa534769625bec6965c9bb42fb`, `c++23`
- 8 TUs/batch, 8 message types/TU, 4 receivers; 5 paired samples, 1 warmup batches/lane/profile, serial compilation.
- Flags: `-O2 -DNDEBUG -pthread`; no PCH, modules, compiler cache, configure or link.
- Warm filesystem cache; each sample recompiles every TU to fresh object files.

| Workload | A median [min, max] s | B median [min, max] s | B vs A | A / B MAD s |
|---|---:|---:|---:|---:|
| wiring | 0.747 [0.722, 0.757] | 1.185 [1.156, 1.304] | +58.68% | 0.002 / 0.030 |
| broker | 4.240 [4.222, 4.543] | 6.862 [6.581, 6.864] | +61.83% | 0.019 / 0.002 |
| umbrella | 2.412 [2.260, 2.460] | 4.408 [4.206, 4.749] | +82.76% | 0.049 / 0.202 |

Positive change means slower. Samples alternate A/B then B/A; raw paired timings and harness/fixture
hashes are in the JSON. Ranges and MAD describe observed noise, not confidence intervals.
Synthetic serial clean-compile cost is not parallel application build time or incremental/no-op time.
Compare revisions within this run; do not compare absolute seconds across different hosts.
