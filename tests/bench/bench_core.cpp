/** Core publish/subscribe benchmarks
 *
 * Built once per macro policy (see tests/CMakeLists.txt) so every policy is measured with identical scenarios:
 *   Sub0Pub_Bench            default: Direct dispatch, no publish context, no filter(), no lock
 *   Sub0Pub_Bench_Checked    SUB0PUB_REENTRANT_CHECK=true (the debug-build default: re-entrancy detected)
 *   Sub0Pub_Bench_Full       SUB0PUB_REENTRANT_SAFE, SUB0PUB_CANCEL, SUB0PUB_FILTER (snapshot, cancel(), filter())
 *   Sub0Pub_Bench_ThreadSafe SUB0PUB_THREAD_SAFE=true, SUB0PUB_FILTER (mutex + snapshot, uncontended)
 * Scenarios for a feature the policy does not enable (filter, cancel) are left out.
 *
 * "Floor" scenarios are hand-written equivalents without Sub0Pub (virtual call loop, std::function
 * loop) that set the bar for what type-erased dispatch to N receivers can cost on this machine.
 */
#define ANKERL_NANOBENCH_IMPLEMENT
#include "sub0pub/sub0pub.hpp"

#include "bench_harness.hpp"
#include "bench_system_info.hpp"

#include <functional>
#include <optional>
#include <vector>

#if defined(_MSC_VER)
#define BENCH_NOINLINE __declspec(noinline)
#else
#define BENCH_NOINLINE __attribute__((noinline))
#endif

/// Floor: the dispatch loop Sub0Pub replaces, as a plain array of interface pointers.
/// Named namespace with two implementations so the optimiser cannot prove a single final override
/// and devirtualise; calls stay indirect exactly as they do inside Broker::publish().
namespace floor_types {
struct IReceiver {
    virtual ~IReceiver() = default;
    virtual bool filter(const int&) noexcept { return true; }
    virtual void receive(const int&) noexcept = 0;
};
struct NoOpReceiver : IReceiver {
    void receive(const int&) noexcept override;
};
struct CountingReceiver : IReceiver {
    int count = 0;
    void receive(const int&) noexcept override;
};
void NoOpReceiver::receive(const int&) noexcept {}
void CountingReceiver::receive(const int&) noexcept { ++count; }

/// Collapse target: what fully devirtualised dispatch reduces to. Receivers are called non-virtually and
/// the compiler is free to inline them; a counting body keeps the work observable.
BENCH_NOINLINE void directCall1(CountingReceiver& a, const int& v) noexcept
{
    a.CountingReceiver::receive(v);
}
BENCH_NOINLINE void directCall8(CountingReceiver (&r)[8], const int& v) noexcept
{
    for (auto& x : r) x.CountingReceiver::receive(v);
}

/// One out-of-line copy of each floor loop, as Broker::publish() is one copy per Data type
BENCH_NOINLINE void receiveAll(IReceiver* const* receivers, std::size_t n, const int& v) noexcept
{
    for (std::size_t i = 0; i < n; ++i) receivers[i]->receive(v);
}
BENCH_NOINLINE void filterReceiveAll(IReceiver* const* receivers, std::size_t n, const int& v) noexcept
{
    for (std::size_t i = 0; i < n; ++i)
        if (receivers[i]->filter(v)) receivers[i]->receive(v);
}
BENCH_NOINLINE void callAll(const std::vector<std::function<void(const int&)>>& fns, const int& v)
{
    for (auto& f : fns) f(v);
}
} // namespace floor_types
using floor_types::IReceiver;
using floor_types::NoOpReceiver;

/// Subscribers get the same treatment as the floor types (named namespace, out-of-line receive(),
/// more than one implementation) so every scenario measures real indirect dispatch, i.e. the
/// worst case / control. Compiler collapse of dispatch (inlining, devirtualisation) is a follow-up.
namespace bench_types {
/// Locked configurations (SUB0PUB_THREAD_SAFE) register explicitly after construction; others in the constructor
template<class Data>
struct Active : sub0::Subscribe<Data> {
    Active() noexcept
    {
#if SUB0PUB_THREAD_SAFE
        this->trySubscribe();
#endif
    }
};
struct NoOpSubscriber final : Active<int> {
    void receive(const int&) noexcept override;
};
struct CountingSubscriber : Active<int> {
    int count = 0;
    void receive(const int&) noexcept override;
};
struct NoOpFloatSub : Active<float> {
    void receive(const float&) noexcept override;
};
struct CountingFloatSub : Active<float> {
    int count = 0;
    void receive(const float&) noexcept override;
};
#if SUB0PUB_FILTER
struct FilteredSubscriber : Active<int> {
    void receive(const int&) noexcept override;
    bool filter(const int& v) noexcept override;
};
#endif
#if SUB0PUB_CANCEL || SUB0PUB_REENTRANT_SAFE || SUB0PUB_THREAD_SAFE
#define BENCH_CANCEL 1
struct CancellingSubscriber : Active<int> {
    void receive(const int&) noexcept override;
};
#endif
void NoOpSubscriber::receive(const int&) noexcept {}
void CountingSubscriber::receive(const int&) noexcept { ++count; }
void NoOpFloatSub::receive(const float&) noexcept {}
void CountingFloatSub::receive(const float&) noexcept { ++count; }
#if SUB0PUB_FILTER
void FilteredSubscriber::receive(const int&) noexcept {}
bool FilteredSubscriber::filter(const int& v) noexcept { return (v & 1) == 0; }
#endif
#if BENCH_CANCEL
void CancellingSubscriber::receive(const int&) noexcept { cancel(); }
#endif
} // namespace bench_types
using namespace bench_types;

namespace {

const char* policyName()
{
#if SUB0PUB_THREAD_SAFE
    return "ThreadSafe (mutex + snapshot, filter)";
#elif SUB0PUB_REENTRANT_SAFE
    return "Full (snapshot, cancel, filter)";
#elif SUB0PUB_REENTRANT_CHECK
    return "Direct + reentrancy check";
#else
    return "Direct (default)";
#endif
}

/// Subscription lifetime operations are out of line for the same reason
BENCH_NOINLINE void createDestroySubscriber() noexcept
{
    NoOpSubscriber sub;
    ankerl::nanobench::doNotOptimizeAway(&sub);
}
BENCH_NOINLINE void reconstruct(std::optional<NoOpSubscriber>& sub) noexcept
{
    sub.reset();
    sub.emplace();
}
BENCH_NOINLINE sub0::SubscribeResult retrySubscribe(sub0::Subscribe<int>& sub) noexcept
{
    return sub.trySubscribe();
}

/// Publish entry points are out of line so every scenario runs the same publish code
struct IntPublisher : sub0::Publish<int> {
    BENCH_NOINLINE void send(int v) noexcept { sub0::publish(*this, v); }
};

struct FloatPublisher : sub0::Publish<float> {
    BENCH_NOINLINE void send(float v) noexcept { sub0::publish(*this, v); }
};


/// Hide the pointers' dynamic types from the optimiser so calls stay virtual, as they are inside Broker::publish()
template<std::size_t N>
void launder(IReceiver* (&receivers)[N]) noexcept
{
    for (auto& r : receivers)
        ankerl::nanobench::doNotOptimizeAway(r);
    bench::clobberMemory();
}

template<std::size_t N>
void publishSubscribers(bench::Harness& h, const char* name)
{
    IntPublisher pub;
    NoOpSubscriber subs[N];
    (void)subs;
    h.run(name, [&] { pub.send(42); });
}

} // namespace

int main()
{
    printSystemInfo();
    std::cout << "Policy: " << policyName() << std::endl
              << "SUB0PUB_MAX_SUBSCRIPTIONS: " << SUB0PUB_MAX_SUBSCRIPTIONS << std::endl << std::endl;

    bench::Harness h;

    // --- Publish throughput by subscriber count ---
    h.title("Publish");
    {
        IntPublisher pub;
        h.run("0 subscribers", [&] { pub.send(42); });
    }
    publishSubscribers<1>(h, "1 subscriber");
    publishSubscribers<2>(h, "2 subscribers");
    publishSubscribers<4>(h, "4 subscribers");
    publishSubscribers<8>(h, "8 subscribers (max default)");

    // --- Filter and cancel ---
    h.title("Filter and cancel");
#if SUB0PUB_FILTER
    {
        IntPublisher pub;
        FilteredSubscriber sub;
        h.run("1 filtered subscriber (pass)", [&] { pub.send(42); });
        h.run("1 filtered subscriber (reject)", [&] { pub.send(43); });
    }
#endif
#if BENCH_CANCEL
    {
        IntPublisher pub;
        CancellingSubscriber first;
        NoOpSubscriber rest[7];
        (void)rest;
        h.run("8 subscribers, first cancels", [&] { pub.send(42); });
    }
#endif

    // --- Multi-type dispatch ---
    h.title("Multi-type dispatch");
    {
        struct MultiPub : sub0::Publish<int>, sub0::Publish<float> {
            BENCH_NOINLINE void sendInt(int v) noexcept { sub0::publish(*this, v); }
            BENCH_NOINLINE void sendFloat(float v) noexcept { sub0::publish(*this, v); }
        };
        MultiPub pub;
        NoOpSubscriber intSub;
        NoOpFloatSub floatSub;
        h.run("int publish (2-type publisher)", [&] { pub.sendInt(42); });
        h.run("float publish (2-type publisher)", [&] { pub.sendFloat(3.14f); });
    }

    // --- Re-entrant publish (nested publish of the same type: every policy supports it) ---
    h.title("Re-entrant publish");
    {
        struct Echo : Active<int> {
            IntPublisher* pub = nullptr;
            void receive(const int& v) noexcept override { if (v > 0) pub->send(v - 1); }
        };
        IntPublisher pub;
        Echo echo;
        echo.pub = &pub;
        h.run("1 subscriber, nested depth 1", [&] { pub.send(1); });
    }

    // --- Subscription lifetime ---
    h.title("Subscription lifetime");
    {
        IntPublisher pub;
        h.run("create + destroy subscriber (empty table)", [&] { createDestroySubscriber(); });
    }
    {
        NoOpSubscriber others[sub0::detail::Broker<int>::cMaxSubscriptions - 1];
        (void)others;
        h.run("create + destroy subscriber (7 others)", [&] { createDestroySubscriber(); });
    }
    {
        // Worst-case removal: the first entry of a full table (order is preserved by shifting the tail).
        // Rotating through the subscribers keeps the one being removed at the front: removing
        // table[0] and re-adding it moves it to the back, making the next index the new front.
        std::optional<NoOpSubscriber> table[sub0::detail::Broker<int>::cMaxSubscriptions];
        for (auto& sub : table) sub.emplace();
        std::size_t next = 0;
        h.run("unsubscribe first of 8 + resubscribe", [&] {
            reconstruct(table[next]);
            next = (next + 1) % sub0::detail::Broker<int>::cMaxSubscriptions;
        });
    }
    {
        NoOpSubscriber subs[sub0::detail::Broker<int>::cMaxSubscriptions]; // table full
        h.run("create + destroy subscriber (table full, rejected)", [&] { createDestroySubscriber(); });
        NoOpSubscriber rejected;
        h.run("trySubscribe() (table full, rejected)", [&] {
            ankerl::nanobench::doNotOptimizeAway(retrySubscribe(rejected));
        });
        h.run("trySubscribe() (already subscribed)", [&] {
            ankerl::nanobench::doNotOptimizeAway(retrySubscribe(subs[0]));
        });
    }

    // --- Floors: equivalent dispatch without Sub0Pub ---
    h.title("Floor (no Sub0Pub)");
    {
        NoOpReceiver r[8];
        IReceiver* one[1] = { &r[0] };
        IReceiver* eight[8];
        for (int i = 0; i < 8; ++i) eight[i] = &r[i];
        launder(one);
        launder(eight);
        floor_types::CountingReceiver counting[8];
        h.run("direct call (collapse target), 1 receiver", [&] { floor_types::directCall1(counting[0], 42); });
        h.run("direct call (collapse target), 8 receivers", [&] { floor_types::directCall8(counting, 42); });
        h.run("virtual receive loop, 1 receiver", [&] { floor_types::receiveAll(one, 1, 42); });
        h.run("virtual receive loop, 8 receivers", [&] { floor_types::receiveAll(eight, 8, 42); });
        h.run("virtual filter+receive loop, 1 receiver", [&] { floor_types::filterReceiveAll(one, 1, 42); });
        h.run("virtual filter+receive loop, 8 receivers", [&] { floor_types::filterReceiveAll(eight, 8, 42); });
        std::vector<std::function<void(const int&)>> fn1(1, [](const int&) {});
        h.run("std::function loop, 1 receiver", [&] { floor_types::callAll(fn1, 42); });
        std::vector<std::function<void(const int&)>> fn8(8, [](const int&) {});
        h.run("std::function loop, 8 receivers", [&] { floor_types::callAll(fn8, 42); });
    }

    return 0;
}
