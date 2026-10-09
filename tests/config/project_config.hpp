#pragma once
/** Example project-wide configuration header (global default override).
 * Named by SUB0PUB_CONFIG_HEADER on the compiler command line (see CMakeLists.txt) so every TU sees it.
 * Zephyr would generate the equivalent from Kconfig (CONFIG_SUB0PUB_*).
 */
// This project opts every type into the full-featured mode: snapshot dispatch, cancel()/routes and filter()
struct ProjectDefaults : sub0::with<sub0::Builtin, sub0::Capacity<4>, sub0::Snapshot, sub0::ThreadLocalContext, sub0::Filter> {};
#define SUB0PUB_DEFAULT_CONFIG ProjectDefaults

#if defined(SUB0PUB_TEST_COUNT_MISMATCHES)
extern int gMismatches;
extern int gViolations;
#undef SUB0PUB_CONFIG_MISMATCH
#define SUB0PUB_CONFIG_MISMATCH(what) ((void)(what), ++gMismatches)
#undef SUB0PUB_REENTRANT_VIOLATION
#define SUB0PUB_REENTRANT_VIOLATION(what) ((void)(what), ++gViolations)
#endif
