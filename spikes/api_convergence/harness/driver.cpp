/** Shootout driver: the collapse-evidence driver (tests/collapse/driver.cpp) plus two measurement modes
 *
 * Every variant defines collapse_setup(), collapse_publish(v) and collapse_teardown() (collapse_case.hpp).
 *
 *   <exe>              The stock run: a warm-up round, then a measured round whose checksum is printed. Under
 *                      valgrind --tool=callgrind each phase is dumped separately, exactly as the stock driver.
 *   <exe> --instr      Windows x64 only. Counts the instructions each phase executes by single-stepping it
 *                      (trap flag + vectored exception handler): exact and deterministic, the substitute for
 *                      callgrind where valgrind does not exist. Prints
 *                      `instr setup=N publish=N publishes=K teardown=N`; publish is the total for K publications.
 *   <exe> --bench N    Times N publications, best of several epochs, and prints `bench ns_per_publish=X`.
 *                      Wall-clock time is noisy: supplemental to the instruction counts, and the workload that
 *                      a sampling profiler (VTune) attaches to.
 *
 * The counted region is the same in every mode and for every variant: the driver's publication loop and the
 * call into collapse_publish() are part of it, so only differences between variants are meaningful.
 */
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "collapse_case.hpp"

#if defined(__has_include)
#if __has_include(<valgrind/callgrind.h>)
#include <valgrind/callgrind.h>
#define SPIKE_CALLGRIND 1
#endif
#endif
#ifndef SPIKE_CALLGRIND
#define SPIKE_CALLGRIND 0
#define CALLGRIND_ZERO_STATS
#define CALLGRIND_DUMP_STATS_AT(x)
#endif

#if defined(_WIN32) && (defined(_M_X64) || defined(__x86_64__))
#define SPIKE_SINGLE_STEP 1
#define WIN32_LEAN_AND_MEAN
#include <intrin.h>
#include <windows.h>
#else
#define SPIKE_SINGLE_STEP 0
#endif

namespace collapse
{
    uint32_t g_state = 0;
    uint32_t g_args = 0;
    uint32_t g_io = 0;
}

namespace
{
    constexpr uint32_t cPublishes = 1000;     // the stock driver's kPublishes: the checksum depends on it
    constexpr uint32_t cSteppedPublishes = 200;
    constexpr uint32_t cBenchEpochs = 9;

    /// Opaque to the optimiser: stops the loop count or values from being folded into the variant
    uint32_t opaque(uint32_t v)
    {
#if defined(__GNUC__)
        asm volatile("" : "+r"(v));
        return v;
#else
        volatile uint32_t barrier = v;
        return barrier;
#endif
    }

    void warmUp()
    {
        collapse_setup();
        for (uint32_t i = 0; i < 8; ++i)
            collapse_publish(opaque(i));
        collapse_teardown();
        collapse::g_state = 0;
        collapse::g_args = 0;
        collapse::g_io = 0;
    }

#if SPIKE_SINGLE_STEP
    constexpr DWORD cTrapFlag = 0x100;
    volatile bool g_stepping = false;
    unsigned long long g_steps = 0;

    LONG CALLBACK onSingleStep(EXCEPTION_POINTERS* info)
    {
        if (info->ExceptionRecord->ExceptionCode != EXCEPTION_SINGLE_STEP)
            return EXCEPTION_CONTINUE_SEARCH;
        if (g_stepping)
        {
            ++g_steps;
            info->ContextRecord->EFlags |= cTrapFlag; // the trap clears it: re-arm for the next instruction
        }
        else
            info->ContextRecord->EFlags &= ~cTrapFlag;
        return EXCEPTION_CONTINUE_EXECUTION;
    }

    __declspec(noinline) void beginStepping()
    {
        g_steps = 0;
        g_stepping = true;
        __writeeflags(__readeflags() | cTrapFlag);
    }

    __declspec(noinline) unsigned long long endStepping()
    {
        g_stepping = false; // the instruction that stores this is the last one counted
        return g_steps;
    }

    int countInstructions()
    {
        AddVectoredExceptionHandler(1, onSingleStep);
        warmUp();

        // The begin/end pair itself executes a fixed number of counted instructions: measure and subtract it
        beginStepping();
        const unsigned long long overhead = endStepping();

        beginStepping();
        collapse_setup();
        const unsigned long long setup = endStepping() - overhead;

        beginStepping();
        for (uint32_t i = 0; i < cSteppedPublishes; ++i)
            collapse_publish(opaque(i));
        const unsigned long long publish = endStepping() - overhead;

        beginStepping();
        collapse_teardown();
        const unsigned long long teardown = endStepping() - overhead;

        std::printf("instr setup=%llu publish=%llu publishes=%u teardown=%llu\n", setup, publish,
                    static_cast<unsigned>(cSteppedPublishes), teardown);
        return 0;
    }
#else
    int countInstructions()
    {
        std::fprintf(stderr, "--instr needs Windows x64; use valgrind --tool=callgrind elsewhere\n");
        return 2;
    }
#endif

    int bench(unsigned long long publishes)
    {
        using Clock = std::chrono::steady_clock;
        warmUp();
        collapse_setup();
        double best = 0.0;
        for (uint32_t epoch = 0; epoch < cBenchEpochs; ++epoch)
        {
            const Clock::time_point start = Clock::now();
            for (unsigned long long i = 0; i < publishes; ++i)
                collapse_publish(opaque(static_cast<uint32_t>(i)));
            const double ns = std::chrono::duration<double, std::nano>(Clock::now() - start).count();
            if (epoch == 0 || ns < best)
                best = ns;
        }
        collapse_teardown();
        std::printf("bench ns_per_publish=%.4f publishes=%llu epochs=%u state=%u\n",
                    best / static_cast<double>(publishes), publishes, static_cast<unsigned>(cBenchEpochs),
                    static_cast<unsigned>(collapse::g_state));
        return 0;
    }
}

int main(int argc, char** argv)
{
    if (argc >= 2 && std::strcmp(argv[1], "--instr") == 0)
        return countInstructions();
    if (argc >= 3 && std::strcmp(argv[1], "--bench") == 0)
        return bench(std::strtoull(argv[2], nullptr, 10));

    warmUp();

    CALLGRIND_ZERO_STATS;
    collapse_setup();
    CALLGRIND_DUMP_STATS_AT("setup");

    for (uint32_t i = 0; i < cPublishes; ++i)
        collapse_publish(opaque(i));
    CALLGRIND_DUMP_STATS_AT("publish");

    collapse_teardown();
    CALLGRIND_DUMP_STATS_AT("teardown");

    std::printf("checksum state=%u args=%u io=%u publishes=%u\n",
                static_cast<unsigned>(collapse::g_state), static_cast<unsigned>(collapse::g_args),
                static_cast<unsigned>(collapse::g_io), static_cast<unsigned>(cPublishes));
    return 0;
}
