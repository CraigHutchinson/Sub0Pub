/** v1 vs v2 comparison: the public header, built against either the v1.0 tag's sub0pub.hpp or the current one.
 *
 * compare_versions.py compiles this file once per header and policy, and passes the column label
 * as CMP_LABEL. Only API common to v1.0 and v2 is used: Subscribe/Publish, virtual receive()/filter(),
 * Subscribe::cancel() and sub0::publish().
 */
#define ANKERL_NANOBENCH_IMPLEMENT
#include <stdexcept> // the v1.0 header uses std::runtime_error without including <stdexcept>
#include "sub0pub/sub0pub.hpp"

#include "cmp_common.hpp"

// filter() and cancel() are opt-in in v2, where the policy (tests/policy/policy.hpp) says which this build has;
// v1.0 always had them
#if defined(CMP_V2)
#define CMP_FILTER POLICY_FILTER
#define CMP_CANCEL (POLICY_CONTEXT || POLICY_SNAPSHOT || POLICY_LOCK)
#define CMP_LOCKED POLICY_LOCK
#else
#define CMP_FILTER 1
#define CMP_CANCEL 1
#define CMP_LOCKED 0
#endif

#include <type_traits>
#include <utility>

#ifndef CMP_LABEL
#define CMP_LABEL "sub0pub.hpp"
#endif

namespace cmp_types {
template<class S, class = void> struct has_try_subscribe : std::false_type {};
template<class S> struct has_try_subscribe<S, std::void_t<decltype(std::declval<S&>().trySubscribe())>> : std::true_type {};

/// v2 locked configurations subscribe explicitly after construction; v1.0 has no trySubscribe()
template<class Data>
struct Active : sub0::Subscribe<Data> {
    Active() noexcept
    {
        if constexpr (CMP_LOCKED && has_try_subscribe<sub0::Subscribe<Data>>::value)
            this->trySubscribe();
    }
};

// Every receiver does the same observable work (one increment), so no implementation can drop the call
struct Counting : Active<int> {
    int count = 0;
    void receive(const int&) noexcept override;
};
struct NoOp : Active<int> { // second implementation: receive() stays a real virtual call
    void receive(const int&) noexcept override;
};
#if CMP_FILTER
struct Filtered : Active<int> {
    int count = 0;
    void receive(const int&) noexcept override;
    bool filter(const int& v) noexcept override;
};
#endif
#if CMP_CANCEL
struct Cancelling : Active<int> {
    int count = 0;
    void receive(const int&) noexcept override;
};
#endif
void NoOp::receive(const int&) noexcept {}
void Counting::receive(const int&) noexcept { ++count; }
#if CMP_FILTER
void Filtered::receive(const int&) noexcept { ++count; }
bool Filtered::filter(const int& v) noexcept { return (v & 1) == 0; }
#endif
#if CMP_CANCEL
void Cancelling::receive(const int&) noexcept { ++count; cancel(); }
#endif

struct Source : sub0::Publish<int> {
    CMP_NOINLINE void send(int v) noexcept { sub0::publish(*this, v); }
};

CMP_NOINLINE void createDestroy() noexcept
{
    Counting sub;
    ankerl::nanobench::doNotOptimizeAway(&sub);
}
} // namespace cmp_types

int main()
{
    using namespace cmp_types;
    bench::Harness h;
    h.title(CMP_LABEL);
    {
        Source pub;
        h.run(cmp::cPublish0, [&] { pub.send(42); });
    }
    {
        Source pub;
        Counting sub;
        h.run(cmp::cPublish1, [&] { pub.send(42); });
    }
    {
        Source pub;
        Counting subs[8];
        (void)subs;
        h.run(cmp::cPublish8, [&] { pub.send(42); });
    }
#if CMP_FILTER
    {
        Source pub;
        Filtered sub;
        h.run(cmp::cFiltered, [&] { pub.send(42); });
    }
#endif
#if CMP_CANCEL
    {
        Source pub;
        Cancelling first;
        Counting rest[7];
        (void)rest;
        h.run(cmp::cCancel8, [&] { pub.send(42); });
    }
#endif
    h.run(cmp::cCreateDestroy, [&] { createDestroy(); });
    NoOp unused; // keeps a second override live, as in tests/bench
    (void)unused;
    return 0;
}
