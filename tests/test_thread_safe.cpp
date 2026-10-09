/** A locked type (sub0::LockWith): concurrent publishers, and subscribers that subscribe explicitly
 *
 * A locked configuration does not subscribe in the Subscribe constructor, so another thread can never dispatch into
 * an object whose derived part is not yet constructed; the most-derived constructor calls trySubscribe().
 */
#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

#include "doctest.h"
#include "sub0pub/sub0pub.hpp"

namespace {

/// The first test publishes while its only subscriber is deliberately not yet active
struct TsMsg { int value; using sub0_config = sub0::config<sub0::LockWith<std::mutex>, sub0::AllowNoReceivers>; };

static_assert(std::is_same_v<sub0::config_t<TsMsg>::Lock, std::mutex>, "LockWith names the lock");
static_assert(sub0::config_t<TsMsg>::dispatch == sub0::Dispatch::Snapshot, "a lock requires Snapshot dispatch");

struct TsPublisher : sub0::Publish<TsMsg>
{
    void send(int value) noexcept { sub0::publish(*this, TsMsg{value}); }
};

/// The pattern for locked configurations: activate last in the constructor, unsubscribe first in the destructor
struct TsCounter : sub0::Subscribe<TsMsg>
{
    TsCounter() noexcept { trySubscribe(); }
    ~TsCounter() { unsubscribe(); }
    std::atomic<int> received{0};
    void receive(const TsMsg&) noexcept override { received.fetch_add(1, std::memory_order_relaxed); }
};

struct TsInactive : sub0::Subscribe<TsMsg>
{
    int received = 0;
    void receive(const TsMsg&) noexcept override { ++received; }
};

} // namespace

TEST_CASE("Thread-safe: a subscriber is inactive until trySubscribe()") {
    TsPublisher pub;
    TsInactive sub;
    CHECK_FALSE(sub.isSubscribed());
    pub.send(1);
    CHECK(sub.received == 0);
    CHECK(sub.trySubscribe() == sub0::SubscribeResult::Subscribed);
    pub.send(2);
    CHECK(sub.received == 1);
    sub.unsubscribe();
}

TEST_CASE("Thread-safe: concurrent publishers with subscriber churn lose nothing for a stable subscriber") {
    TsCounter stable;
    static constexpr int cN = 5000; // static: used by a capture-less lambda (MSVC requires capturing a local)
    std::atomic<bool> publishing{true};
    std::thread churn([&] {
        while (publishing.load(std::memory_order_relaxed))
        {
            TsCounter transient;
            std::this_thread::yield();
        }
    });
    std::vector<std::thread> publishers;
    for (int t = 0; t < 3; ++t)
        publishers.emplace_back([] { TsPublisher p; for (int i = 0; i < cN; ++i) p.send(i); });
    for (auto& p : publishers)
        p.join();
    publishing.store(false, std::memory_order_relaxed);
    churn.join();
    CHECK(stable.received.load() == 3 * cN);
}
