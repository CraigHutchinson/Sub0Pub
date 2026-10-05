#pragma once
/** v1 vs v2 comparison: shared scenario names and controls (see compare_versions.py)
 *
 * Every implementation runs the same scenarios under the same control conditions as tests/bench:
 * publish and lifetime entry points are out of line, and every receiver's receive() is out of line.
 * A scenario an implementation cannot express is left out; the report shows it as n/a.
 */
#include "bench_harness.hpp"

#if defined(_MSC_VER)
#define CMP_NOINLINE __declspec(noinline)
#else
#define CMP_NOINLINE __attribute__((noinline))
#endif

namespace cmp {
constexpr const char* cPublish0 = "publish, 0 subscribers";
constexpr const char* cPublish1 = "publish, 1 subscriber";
constexpr const char* cPublish8 = "publish, 8 subscribers";
constexpr const char* cFiltered = "publish, 1 filtered subscriber (pass)";
constexpr const char* cCancel8 = "publish, 8 subscribers, first cancels";
constexpr const char* cCreateDestroy = "create + destroy subscriber";
} // namespace cmp
