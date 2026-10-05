#pragma once
/** Benchmark harness: wall-clock timing via nanobench, or exact instruction counts under callgrind
 *
 * Normal run: each scenario is timed with nanobench (ns/op, noisy, machine-specific).
 * Under valgrind --tool=callgrind: each scenario is executed a fixed number of times between
 * CALLGRIND_ZERO_STATS and CALLGRIND_DUMP_STATS_AT(name). The per-dump instruction totals divided
 * by the iteration count give deterministic instructions/op, independent of machine noise, which
 * makes them usable as a regression bar. See tests/bench/run_baseline.py.
 */
#include "nanobench.h"

#if defined(__has_include)
#if __has_include(<valgrind/callgrind.h>)
#include <valgrind/callgrind.h>
#define SUB0PUB_BENCH_CALLGRIND 1
#endif
#endif
#ifndef SUB0PUB_BENCH_CALLGRIND
#define SUB0PUB_BENCH_CALLGRIND 0
#endif

#if defined(_MSC_VER) && !defined(__clang__)
#include <intrin.h>
#endif
#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>

namespace bench {

/// Compiler-only memory barrier (no instructions): stops the optimiser hoisting loop-invariant work
/// (e.g. a publish to 0 subscribers) out of the measurement loop. Applied identically to every scenario.
inline void clobberMemory() noexcept
{
#if defined(_MSC_VER) && !defined(__clang__)
    _ReadWriteBarrier();
#else
    asm volatile("" : : : "memory");
#endif
}

/// Iterations per scenario when counting instructions (loop overhead is ~3 instructions/iteration)
constexpr uint32_t cInstrIterations = 10000;

class Harness
{
public:
    Harness()
    {
        bench_.warmup(1000).minEpochTime(std::chrono::milliseconds(2)).epochs(15);
#if SUB0PUB_BENCH_CALLGRIND
        instr_ = RUNNING_ON_VALGRIND != 0;
#endif
    }

    /** @return True when running under callgrind (instruction count mode) */
    bool counting() const noexcept { return instr_; }

    void title(const std::string& title)
    {
        title_ = title;
        if (!instr_)
            bench_.title(title);
    }

    /** Measure one scenario; name is qualified with the current title for callgrind dumps */
    template<typename Op>
    void run(const std::string& name, Op&& op)
    {
        if (instr_)
        {
#if SUB0PUB_BENCH_CALLGRIND
            for (uint32_t i = 0; i < 100; ++i) op(); // warm caches/lazy init, as nanobench warmup
            const std::string label = title_ + " | " + name;
            CALLGRIND_ZERO_STATS;
            for (uint32_t i = 0; i < cInstrIterations; ++i)
            {
                op();
                clobberMemory();
            }
            CALLGRIND_DUMP_STATS_AT(label.c_str());
#endif
        }
        else
        {
            bench_.run(name, [&] { op(); clobberMemory(); });
        }
    }

    ankerl::nanobench::Bench& nanobench() noexcept { return bench_; }

private:
    ankerl::nanobench::Bench bench_;
    std::string title_;
    bool instr_ = false;
};

} // namespace bench
