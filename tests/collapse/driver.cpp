/** Collapse evidence driver: runs one case variant and reports per-phase instruction counts and a checksum
 *
 * Under valgrind --tool=callgrind each phase is dumped separately (client requests):
 *   "setup" / "teardown": one construction / destruction, measured on the second round (first round warms up)
 *   "publish":            kPublishes publications
 * The printed checksum (observable state, argument evaluations) must be identical for every variant of a case.
 */
#include "collapse_case.hpp"

#if !defined(COLLAPSE_NO_STDIO)
#include <cstdio>
#endif

#if defined(__has_include)
#if __has_include(<valgrind/callgrind.h>)
#include <valgrind/callgrind.h>
#define COLLAPSE_CALLGRIND 1
#endif
#endif
#ifndef COLLAPSE_CALLGRIND
#define COLLAPSE_CALLGRIND 0
#define CALLGRIND_ZERO_STATS
#define CALLGRIND_DUMP_STATS_AT(x)
#endif

namespace collapse
{
    uint32_t g_state = 0;
    uint32_t g_args = 0;
    uint32_t g_io = 0;
}

namespace
{
    constexpr uint32_t kPublishes = 1000;

    /// Opaque to the optimiser: stops the loop count or values from being folded into the variant
    uint32_t opaque(uint32_t v)
    {
#if defined(__GNUC__)
        asm volatile("" : "+r"(v));
        return v;
#else
        // MSVC x64 has no inline assembly: a volatile round trip is the equivalent optimisation barrier
        volatile uint32_t barrier = v;
        return barrier;
#endif
    }
}

int main()
{
    // Round 1: warm up (first-touch, lazy binding, caches), then reset the observable state
    collapse_setup();
    for (uint32_t i = 0; i < 8; ++i)
        collapse_publish(opaque(i));
    collapse_teardown();
    collapse::g_state = 0;
    collapse::g_args = 0;
    collapse::g_io = 0;

    // Round 2: measured
    CALLGRIND_ZERO_STATS;
    collapse_setup();
    CALLGRIND_DUMP_STATS_AT("setup");

    for (uint32_t i = 0; i < kPublishes; ++i)
        collapse_publish(opaque(i));
    CALLGRIND_DUMP_STATS_AT("publish");

    collapse_teardown();
    CALLGRIND_DUMP_STATS_AT("teardown");

#if defined(COLLAPSE_NO_STDIO)
    // Bare-metal builds are analysed statically (final ELF); keep stdio out of the image
    return static_cast<int>(collapse::g_state ^ collapse::g_args ^ collapse::g_io);
#else
    std::printf("checksum state=%u args=%u io=%u publishes=%u\n",
                static_cast<unsigned>(collapse::g_state), static_cast<unsigned>(collapse::g_args),
                static_cast<unsigned>(collapse::g_io), static_cast<unsigned>(kPublishes));
    return 0;
#endif
}
