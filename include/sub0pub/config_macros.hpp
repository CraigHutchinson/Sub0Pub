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

#ifndef SUB0PUB_MAX_SUBSCRIPTIONS
#define SUB0PUB_MAX_SUBSCRIPTIONS 8 ///< Fixed subscription table size per Broker<T>. Override globally or per-TU.
#endif

/** Inlining for the static-wiring delivery chain (sub0pub/wiring): not a configuration option.
 * MSVC's /O2 inliner stops at a delivery chain of many receivers and leaves StaticWiring::publish out of line
 * (32 receivers: 169 publish-path instructions against 69 hand-written; docs/EVIDENCE.md), so the
 * wiring asks for inlining explicitly there, and so do the two thin functions that lead to it from a StaticTo
 * type (sub0::publish() and its Publish handle). Every other compiler gets plain `inline`, so their code is unchanged.
 */
#if defined(_MSC_VER) && !defined(__clang__)
#define SUB0PUB_FORCE_INLINE __forceinline
#else
#define SUB0PUB_FORCE_INLINE inline
#endif

/** Empty-base layout for a class with several empty bases: not a configuration option.
 * The MSVC ABI gives every empty base after the first a byte of its own, and value-initialisation then writes
 * those bytes. A receiver of statically wired types derives from one empty Subscribe<T> per type, so the library's
 * own multi-base helper (SubscribeAll) asks for the packed layout; a class of yours with several Subscribe<T> or
 * Publish<T> bases can do the same. Expands to nothing elsewhere, where the layout is already packed.
 */
#if defined(_MSC_VER)
#define SUB0PUB_EMPTY_BASES __declspec(empty_bases)
#else
#define SUB0PUB_EMPTY_BASES
#endif

#if defined(SUB0PUB_REENTRANT_SAFE) || defined(SUB0PUB_CANCEL) || defined(SUB0PUB_FILTER) || defined(SUB0PUB_THREAD_SAFE)
#error "Sub0Pub policy macros were removed: use per-type options or SUB0PUB_CONFIG_HEADER (see MIGRATION.md)"
#endif

/* Default configuration: the cheapest correct dispatch. Every feature that costs something is an option of a Data
 * type's configuration (sub0pub/config.hpp), and using one without it is detected: at compile time where possible,
 * otherwise by a debug-build check.
 *   sub0::Snapshot             snapshot dispatch   detected by SUB0PUB_REENTRANT_CHECK (debug)
 *   sub0::ThreadLocalContext   publish context     cancel(), Route and publish reports do not compile without it
 *   sub0::Filter               filter()            a subscriber declaring filter() does not compile without it
 *   sub0::LockWith<L>          lock                detected by SUB0PUB_THREAD_CHECK (debug)
 * A publication is expected to reach somebody: one that reaches no receiver is detected by
 * SUB0PUB_NO_RECEIVERS_CHECK (debug), or at compile time for a statically wired type.
 * A project that wants an option for every Data type says so once, in its SUB0PUB_CONFIG_HEADER.
 */

/** Detect re-entrancy that Direct dispatch does not support
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
 * Publish, subscribe and unsubscribe of a Data type without a lock (sub0::LockWith) must not
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

/** Record what a run publishes and delivers, and report it (sub0pub/audit.hpp)
 * An audit build keeps a ledger per message type and reports, at exit and through sub0::auditReport(), the
 * publications that reached nobody, the subscribers that were never called, refused subscriptions, subscribers a
 * StaticTo list forgot, and which brokered types had receivers that never changed (candidates for StaticTo).
 * Default: disabled, and then none of it exists. It is a property of the whole program: define it for every
 * translation unit, from the build system. It needs RTTI to name runtime subscribers, and is not for release builds:
 * every publication, delivery and subscription takes a process-wide lock.
 * While it is enabled the two run-time checks below default to off, so that a run completes and the audit reports
 * everything they would have stopped at; SUB0PUB_AUDIT_PRINT(line) and SUB0PUB_AUDIT_EXIT(findings) say where the
 * report goes and what follows it (sub0pub/audit.hpp).
 */
#ifndef SUB0PUB_AUDIT
#define SUB0PUB_AUDIT false
#endif

/** Detect a publication that reaches no receiver
 * Publishing a Data type that nobody is subscribed to is almost always a mistake: a publisher that starts before
 * its subscribers, a subscriber that was never constructed, a translation unit that does not see a type's
 * configuration. With this check enabled such a publication calls SUB0PUB_NO_RECEIVERS(what).
 * Default: enabled in debug builds (SUB0PUB_ASSERT and no NDEBUG), disabled in release builds and in an audit build
 * (SUB0PUB_AUDIT), which reports such publications instead of stopping at the first. Define it true to
 * keep the check in release builds: it replaces the dispatch loop's own entry test, so a publication that has
 * receivers does not pay for it. One type can choose differently: sub0::AllowNoReceivers where an absent receiver
 * is expected (a diagnostic stream, a plug-in loaded at run time), sub0::ReportNoReceivers to check it in every build.
 * A statically wired type (sub0::StaticTo) is checked at compile time instead, whatever this is set to.
 */
#ifndef SUB0PUB_NO_RECEIVERS_CHECK
#if SUB0PUB_ASSERT && !defined(NDEBUG) && !SUB0PUB_AUDIT
#define SUB0PUB_NO_RECEIVERS_CHECK true
#else
#define SUB0PUB_NO_RECEIVERS_CHECK false
#endif
#endif

/** Action on a publication that reached no receiver (see SUB0PUB_NO_RECEIVERS_CHECK)
 * @param what  Null-terminated description of the failure
 * Default asserts (debug) then aborts. Override to log or count instead; if it returns, the publication simply had
 * no effect.
 */
#ifndef SUB0PUB_NO_RECEIVERS
#define SUB0PUB_NO_RECEIVERS(what) do { assert(!(what)); std::abort(); } while(false)
#endif

/** Detect a subscriber of a statically wired type that the type's sub0::StaticTo list does not name
 * Such a subscriber is never called: the list is the whole set of receivers. With this check enabled its
 * construction calls SUB0PUB_UNLISTED_RECEIVER(what).
 * Default: enabled in debug builds (SUB0PUB_ASSERT and no NDEBUG), but not in an audit build (SUB0PUB_AUDIT), which
 * reports such a subscriber instead. Every translation unit that constructs subscribers of a type must agree on it.
 * A unit test that constructs one receiver on its own, outside the list its type names, switches it off or
 * overrides the action.
 */
#ifndef SUB0PUB_UNLISTED_CHECK
#if SUB0PUB_ASSERT && !defined(NDEBUG) && !SUB0PUB_AUDIT
#define SUB0PUB_UNLISTED_CHECK true
#else
#define SUB0PUB_UNLISTED_CHECK false
#endif
#endif

/** Action on a detected unlisted subscriber (see SUB0PUB_UNLISTED_CHECK). Default asserts, then aborts. */
#ifndef SUB0PUB_UNLISTED_RECEIVER
#define SUB0PUB_UNLISTED_RECEIVER(what) do { assert(!(what)); std::abort(); } while(false)
#endif

#if SUB0PUB_STD
#include <ostream> //< std::ostream
#include <istream> //< std::istream
#endif

#endif // CROG_SUB0PUB_CONFIG_MACROS_HPP
