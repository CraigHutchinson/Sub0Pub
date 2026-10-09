/** Whole-program policies for the benchmarks and footprint report.
 *
 * Named as SUB0PUB_CONFIG_HEADER by each tool, which is how a project gives every Data type an option. The tool
 * chooses the options with POLICY_SNAPSHOT, POLICY_CONTEXT, POLICY_FILTER and POLICY_LOCK (each 0 or 1), so one
 * scenario source is measured under several defaults. With all four 0 this is the library's own default.
 */
#pragma once

#ifndef POLICY_SNAPSHOT
#define POLICY_SNAPSHOT 0
#endif
#ifndef POLICY_CONTEXT
#define POLICY_CONTEXT 0
#endif
#ifndef POLICY_FILTER
#define POLICY_FILTER 0
#endif
#ifndef POLICY_LOCK
#define POLICY_LOCK 0
#endif

#include <type_traits>
#if POLICY_LOCK
#include <mutex>
#endif

namespace policy
{
#if POLICY_LOCK
    using Mutex = std::mutex;
#else
    using Mutex = sub0::NoLock;
#endif

    /// Base with Option applied when the tool asked for it
    template<bool On, class Option, class Base>
    using when = std::conditional_t<On, sub0::with<Base, Option>, Base>;

    struct Defaults
        : when<POLICY_LOCK, sub0::LockWith<Mutex>,
          when<POLICY_FILTER, sub0::Filter,
          when<POLICY_SNAPSHOT, sub0::Snapshot,
          when<POLICY_CONTEXT, sub0::ThreadLocalContext, sub0::Builtin>>>>
    {};
} // namespace policy

#define SUB0PUB_DEFAULT_CONFIG policy::Defaults
