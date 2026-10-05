# Sub0Pub compile-time A/B

- UTC: 2026-09-30T14:02:15.107350+00:00
- Compiler: `g++ (Ubuntu 13.3.0-6ubuntu2~24.04) 13.3.0`
- Host: `Linux-6.18.44-x86_64-with-glibc2.39`; CPU: INTEL(R) XEON(R) PLATINUM 8573C; logical CPUs: 9
- A: `ab3622277018b7d18cc507afad64af1d1bfbe6a6`, `c++23`
- B: `1e8f1dcef8ed29fa534769625bec6965c9bb42fb`, `c++23`
- 8 TUs/batch, 8 message types/TU, 4 receivers; 5 paired samples, 1 warmup batches/lane/profile, serial compilation.
- Flags: `-O2 -DNDEBUG -pthread`; no PCH, modules, compiler cache, configure or link.
- Warm filesystem cache; each sample recompiles every TU to fresh object files.

| Workload | A median [min, max] s | B median [min, max] s | B vs A | A / B MAD s |
|---|---:|---:|---:|---:|
| wiring | 2.407 [2.294, 2.805] | 2.054 [1.969, 2.747] | -14.64% | 0.097 / 0.064 |
| broker | 10.972 [10.375, 15.253] | 11.074 [10.405, 13.946] | +0.93% | 0.597 / 0.450 |
| umbrella | 7.743 [7.101, 8.522] | 8.983 [7.097, 11.524] | +16.00% | 0.643 / 0.177 |

Positive change means slower. Samples alternate A/B then B/A; raw paired timings and harness/fixture
hashes are in the JSON. Ranges and MAD describe observed noise, not confidence intervals.
Synthetic serial clean-compile cost is not parallel application build time or incremental/no-op time.
Compare revisions within this run; do not compare absolute seconds across different hosts.
