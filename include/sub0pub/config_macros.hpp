/** Sub0Pub: Configuration macros: every SUB0PUB_* default, defined once and read at include time
 * @remark Part of Sub0Pub (https://github.com/BareCpper/Sub0Pub), MIT License: see LICENSE.md.
 *         Included by the umbrella header <sub0pub/sub0pub.hpp>.
 */
#ifndef CROG_SUB0PUB_CONFIG_MACROS_HPP
#define CROG_SUB0PUB_CONFIG_MACROS_HPP

// Some C++23-mode compilers report a draft date. MSVC reports its selected mode via _MSVC_LANG.
#if defined(_MSVC_LANG)
#  if _MSVC_LANG <= 202002L
#    error "Sub0Pub v2 requires C++23; link Sub0Pub::Sub0Pub or enable C++23 mode"
#  endif
#elif __cplusplus <= 202002L
#  error "Sub0Pub v2 requires C++23; link Sub0Pub::Sub0Pub or enable C++23 mode"
#endif

#include <cassert>
#include <cstdlib>

/** Logging output for event tracing
 * Define SUB0PUB_TRACE=true to enable message logging to std::cout for event trace, SUB0PUB_TRACE=false
 */
#ifndef SUB0PUB_TRACE
#define SUB0PUB_TRACE false ///< Disable Trace logging to std::cout by default
#endif

/** Assertion based error handling 
 * Define SUB0PUB_ASSERT=true to enable assertion checks for events, SUB0PUB_ASSERT=false to disable
 */
#ifndef SUB0PUB_ASSERT
#define SUB0PUB_ASSERT true ///< Enable assertion tests by default
#endif

#ifndef SUB0PUB_STD
#define SUB0PUB_STD false ///< Use STD ostream and IStream types (May increase binary compiled size)
#endif

#ifndef SUB0PUB_TYPEIDNAME
#define SUB0PUB_TYPEIDNAME false ///< Types given unique/user-defined type index and string name for diagnostics and IPC
#endif

#ifndef SUB0PUB_THREAD_SAFE
#define SUB0PUB_THREAD_SAFE false ///< Optional mutex guard for multi-threaded pub/sub (e.g. FreeRTOS dual-core)
#endif

#ifndef SUB0PUB_MAX_SUBSCRIPTIONS
#define SUB0PUB_MAX_SUBSCRIPTIONS 8 ///< Fixed subscription table size per Broker<T>. Override globally or per-TU.
#endif

/** Inlining for the static-wiring delivery chain (sub0pub/wiring): not a configuration option.
 * MSVC's /O2 inliner stops at a delivery chain of many receivers and leaves StaticWiring::publish out of line
 * (32 receivers: 169 publish-path instructions against 69 hand-written; docs/EVIDENCE.md), so the
 * wiring asks for inlining explicitly there. Every other compiler gets plain `inline`, so their code is unchanged.
 */
#if defined(_MSC_VER) && !defined(__clang__)
#define SUB0PUB_FORCE_INLINE __forceinline
#else
#define SUB0PUB_FORCE_INLINE inline
#endif

/* Default configuration: the cheapest correct dispatch. Every feature that costs something is opt-in, and
 * using one without opting in is detected: at compile time where possible, otherwise by a debug-build check.
 *   SUB0PUB_REENTRANT_SAFE  snapshot dispatch   detected by SUB0PUB_REENTRANT_CHECK (debug)
 *   SUB0PUB_CANCEL          publish context     cancel(), Route and publish reports do not compile without it
 *   SUB0PUB_FILTER          filter()            a subscriber declaring filter() does not compile without it
 *   SUB0PUB_THREAD_SAFE     lock                detected by SUB0PUB_THREAD_CHECK (debug)
 * These macros set the default for every Data type; one type can choose differently (sub0::config).
 */

#ifndef SUB0PUB_REENTRANT_SAFE
#define SUB0PUB_REENTRANT_SAFE false ///< Snapshot dispatch: subscribe or unsubscribe a Data type from inside its own
                                     ///< receive() (including destroying the receiving subscriber). Costs a copy of
                                     ///< the table and a publish context per publish.
#endif

#ifndef SUB0PUB_CANCEL
#define SUB0PUB_CANCEL false ///< Publish context: cancel(), Route (transport endpoints) and publish reports.
                             ///< Costs a thread_local frame per publish.
#endif

#ifndef SUB0PUB_FILTER
#define SUB0PUB_FILTER false ///< Subscribe<Data>::filter(): a virtual call per subscriber per publish.
#endif

/** Detect re-entrancy that Direct dispatch (SUB0PUB_REENTRANT_SAFE=false) does not support
 * Subscribing or unsubscribing (including destroying) a subscriber of a Data type from within a receive() of that
 * same Data type on the same thread is a contract violation of Direct dispatch; nested publish is supported. With
 * this check enabled the violation calls SUB0PUB_REENTRANT_VIOLATION(what).
 * Default: enabled in debug builds (SUB0PUB_ASSERT and no NDEBUG), disabled in release builds.
 * Define SUB0PUB_REENTRANT_CHECK true to keep the check in release builds (a thread_local frame per publish).
 * Has no effect with snapshot dispatch, which supports these.
 */
#ifndef SUB0PUB_REENTRANT_CHECK
#if SUB0PUB_ASSERT && !defined(NDEBUG)
#define SUB0PUB_REENTRANT_CHECK true
#else
#define SUB0PUB_REENTRANT_CHECK false
#endif
#endif

/** Action on a detected re-entrancy violation (see SUB0PUB_REENTRANT_CHECK)
 * @param what  Null-terminated description of the violation
 * Default asserts (debug) then aborts, so it also stops a release build that opted in to the check.
 * Override to log or count instead; if it returns, the operation continues unguarded.
 */
#ifndef SUB0PUB_REENTRANT_VIOLATION
#define SUB0PUB_REENTRANT_VIOLATION(what) do { assert(!(what)); std::abort(); } while(false)
#endif

/** Detect a Data type used from two threads at once without a lock
 * Publish, subscribe and unsubscribe of a Data type without a lock (SUB0PUB_THREAD_SAFE or sub0::LockWith) must not
 * overlap on different threads. With this check enabled an overlap calls SUB0PUB_THREAD_VIOLATION(what).
 * Default: enabled in debug builds (SUB0PUB_ASSERT and no NDEBUG). It detects overlaps that happen, not every race.
 */
#ifndef SUB0PUB_THREAD_CHECK
#if SUB0PUB_ASSERT && !defined(NDEBUG)
#define SUB0PUB_THREAD_CHECK true
#else
#define SUB0PUB_THREAD_CHECK false
#endif
#endif

/** Action on a detected unlocked concurrent use (see SUB0PUB_THREAD_CHECK). Default asserts, then aborts. */
#ifndef SUB0PUB_THREAD_VIOLATION
#define SUB0PUB_THREAD_VIOLATION(what) do { assert(!(what)); std::abort(); } while(false)
#endif

/** Debug diagnostic for a Data type resolving to different configurations in different translation units
 * Default: enabled in debug builds (SUB0PUB_ASSERT and no NDEBUG). Consistent configuration visibility is a build
 * contract (see sub0::config_t); this check only reports violations it observes at runtime.
 */
#ifndef SUB0PUB_CHECK_CONFIG
#if SUB0PUB_ASSERT && !defined(NDEBUG)
#define SUB0PUB_CHECK_CONFIG true
#else
#define SUB0PUB_CHECK_CONFIG false
#endif
#endif

/** Action on a detected configuration mismatch (see SUB0PUB_CHECK_CONFIG). Default asserts, then aborts. */
#ifndef SUB0PUB_CONFIG_MISMATCH
#define SUB0PUB_CONFIG_MISMATCH(what) do { assert(!(what)); std::abort(); } while(false)
#endif

/** Action when a Domain is destroyed while handles are still bound to it. Default asserts, then aborts. */
#ifndef SUB0PUB_DOMAIN_LIFETIME
#define SUB0PUB_DOMAIN_LIFETIME(what) do { assert(!(what)); std::abort(); } while(false)
#endif

/** Helper macro for stringifying value using compiler preprocessor
 * e.g. SUB0PUB_STRINGIFY_HELPER(123) == "123", SUB0PUB_STRINGIFY_HELPER(FooBar) == "FooBar"
 * @param  x  A value whos value will be converted to string e.g. FooBar == "FooBar", 123 = "123"
 */
#define SUB0PUB_STRINGIFY_HELPER(x) #x

/** Helper macro for stringifying define using compiler preprocessor
 * e.g. SUB0PUB_STRINGIFY_HELPER(__LINE__) == "123??"
 * @param  x  A macro definition whos value will be converted to string  e.g. __LINE__ == "123??"
 */
#define SUB0PUB_STRINGIFY(x) SUB0PUB_STRINGIFY_HELPER(x)

#if SUB0PUB_STD
#include <ostream> //< std::ostream
#include <istream> //< std::istream
#endif

/// @todo Trace interface - currently std::cout only!!
#if SUB0PUB_TRACE
#include <iostream>
#endif

#endif // CROG_SUB0PUB_CONFIG_MACROS_HPP
