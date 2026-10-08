/** The guarantee each configuration option claims (docs/DESIGN.md, "Per-type configuration").
 * One behaviour per test, per option, where no other test demonstrated it. */
#include "doctest.h"
#include "sub0pub/sub0pub.hpp"

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

extern int gViolations; // project_config.hpp counts SUB0PUB_REENTRANT_VIOLATION instead of aborting

namespace {

struct SpinLock
{
    std::atomic_flag flag = ATOMIC_FLAG_INIT;
    void lock() noexcept { while (flag.test_and_set(std::memory_order_acquire)) std::this_thread::yield(); }
    void unlock() noexcept { flag.clear(std::memory_order_release); }
};

template<int Id, class... Opts>
struct Msg
{
    int value;
    using sub0_config = sub0::config<Opts...>;
};

using SnapMsg    = Msg<1, sub0::Snapshot, sub0::Capacity<8>>;
using DirectMsg  = Msg<2, sub0::Direct, sub0::Capacity<8>>;
using CheckedMsg = Msg<3, sub0::DirectChecked, sub0::StaticContext, sub0::Capacity<8>>;
using StaticMsg  = Msg<4, sub0::Snapshot, sub0::StaticContext, sub0::Capacity<8>>;
using LockedMsg  = Msg<5, sub0::LockWith<SpinLock>, sub0::Snapshot, sub0::ThreadLocalContext, sub0::Capacity<8>>;
using ChurnMsg   = Msg<6, sub0::LockWith<SpinLock>, sub0::Snapshot, sub0::ThreadLocalContext, sub0::Capacity<8>>;

template<class Data>
struct Source : sub0::Publish<Data>
{
    void send(const Data& d) noexcept { sub0::publish(*this, d); }
};

/// Records deliveries; an optional action runs inside receive() (mutations during dispatch)
template<class Data>
struct Probe final : sub0::Subscribe<Data>
{
    Probe() noexcept { if constexpr (sub0::detail::cConcurrent<sub0::config_t<Data>>) this->trySubscribe(); }
    ~Probe() { this->unsubscribe(); }
    std::atomic<int> received{0}; // concurrent configurations call a receiver from several publisher threads at once
    std::function<void(const Data&)> action;
    void receive(const Data& d) noexcept override
    {
        received.fetch_add(1, std::memory_order_relaxed);
        if (action)
            action(d);
    }
};

} // namespace

// --- Dispatch = Snapshot: the table may change during a dispatch; the dispatch sees the table as it was ---

TEST_CASE("axes: Snapshot, a subscriber added during dispatch is called from the next publish") {
    Source<SnapMsg> pub;
    std::unique_ptr<Probe<SnapMsg>> late;
    Probe<SnapMsg> first;
    first.action = [&](const SnapMsg&) { if (!late) late = std::make_unique<Probe<SnapMsg>>(); };
    pub.send(SnapMsg{1});
    REQUIRE(late);
    CHECK(late->received == 0);   // not part of the snapshot being delivered
    pub.send(SnapMsg{2});
    CHECK(late->received == 1);
}

TEST_CASE("axes: Snapshot, a later subscriber unsubscribed during dispatch is not called") {
    Source<SnapMsg> pub;
    Probe<SnapMsg> first;
    Probe<SnapMsg> second;
    first.action = [&](const SnapMsg&) { second.unsubscribe(); };
    pub.send(SnapMsg{1});
    CHECK(first.received == 1);
    CHECK(second.received == 0);  // removed from this dispatch's snapshot, not only from the table
}

TEST_CASE("axes: Snapshot, re-entrant publish of the same type is delivered (nested)") {
    Source<SnapMsg> pub;
    Probe<SnapMsg> echo;
    echo.action = [&](const SnapMsg& m) { if (m.value > 0) pub.send(SnapMsg{m.value - 1}); };
    pub.send(SnapMsg{2});
    CHECK(echo.received == 3);    // 2, then nested 1, then nested 0
}

// --- Dispatch = Direct: iterates the live table; re-entrant publish is fine, table mutation is not supported ---

TEST_CASE("axes: Direct, re-entrant publish of the same type is delivered (no table mutation)") {
    Source<DirectMsg> pub;
    Probe<DirectMsg> echo;
    echo.action = [&](const DirectMsg& m) { if (m.value > 0) pub.send(DirectMsg{m.value - 1}); };
    pub.send(DirectMsg{2});
    CHECK(echo.received == 3);
}

// --- Dispatch = DirectChecked: a change to the table during its own dispatch is reported; nesting is not ---

TEST_CASE("axes: DirectChecked, nested publish is allowed and subscribe during dispatch is reported") {
    Source<CheckedMsg> pub;
    Probe<CheckedMsg> probe;
    gViolations = 0;
    probe.action = [&](const CheckedMsg& m) { if (m.value > 0) pub.send(CheckedMsg{0}); };
    pub.send(CheckedMsg{1});
    CHECK(gViolations == 0);
    CHECK(probe.received == 2);

    gViolations = 0;
    std::unique_ptr<Probe<CheckedMsg>> late;
    probe.action = [&](const CheckedMsg&) { if (!late) late = std::make_unique<Probe<CheckedMsg>>(); };
    pub.send(CheckedMsg{0});
    CHECK(gViolations == 1);
    probe.action = nullptr;
}

// --- Context = Static: cancel() works without TLS (single-threaded publishers) ---

TEST_CASE("axes: StaticContext, cancel() stops the rest of the current publication only") {
    Source<StaticMsg> pub;
    Probe<StaticMsg> gate;
    Probe<StaticMsg> after;
    gate.action = [&](const StaticMsg& m) { if (m.value == 0) gate.cancel(); };
    pub.send(StaticMsg{0});
    CHECK(after.received == 0);
    pub.send(StaticMsg{1});
    CHECK(after.received == 1);   // the next publication is not affected
}

// --- Lock + ThreadLocalContext: concurrent publishers are isolated from each other's cancel() ---
// (Lock + StaticContext is rejected at compile time: compile_fail/cf_config_lock_static_context.cpp.)

TEST_CASE("axes: Lock, one publisher thread's cancel() never affects another thread's publication") {
    Probe<LockedMsg> gate;
    std::atomic<int> delivered{0};
    struct Counter : sub0::Subscribe<LockedMsg>
    {
        explicit Counter(std::atomic<int>& n) noexcept : count(n) { trySubscribe(); }
        ~Counter() { unsubscribe(); }
        void receive(const LockedMsg& m) noexcept override { if (m.value >= 0) count.fetch_add(1, std::memory_order_relaxed); }
        std::atomic<int>& count;
    } counter(delivered);
    gate.action = [&](const LockedMsg& m) { if (m.value < 0) gate.cancel(); };

    constexpr int cN = 20000;
    std::thread keeps([&] { Source<LockedMsg> s; for (int i = 0; i < cN; ++i) s.send(LockedMsg{1}); });
    std::thread cancels([&] { Source<LockedMsg> s; for (int i = 0; i < cN; ++i) s.send(LockedMsg{-1}); });
    keeps.join();
    cancels.join();
    CHECK(delivered.load() == cN);
    gate.action = nullptr;
}

// --- Lock: several publishers while subscribers come and go on another thread ---

TEST_CASE("axes: Lock, concurrent publishers with subscribe/unsubscribe churn: stable subscriber loses nothing") {
    std::atomic<int> stableCount{0};
    struct Stable : sub0::Subscribe<ChurnMsg>
    {
        explicit Stable(std::atomic<int>& n) noexcept : count(n) { trySubscribe(); }
        ~Stable() { unsubscribe(); }
        void receive(const ChurnMsg&) noexcept override { count.fetch_add(1, std::memory_order_relaxed); }
        std::atomic<int>& count;
    } stable(stableCount);

    constexpr int cN = 10000;
    std::atomic<bool> publishing{true};
    std::thread churn([&] {
        while (publishing.load(std::memory_order_relaxed))
        {
            Probe<ChurnMsg> transient; // subscribes at construction, unsubscribes first at destruction
            std::this_thread::yield();
        }
    });
    std::vector<std::thread> publishers;
    for (int t = 0; t < 3; ++t)
        publishers.emplace_back([&] { Source<ChurnMsg> s; for (int i = 0; i < cN; ++i) s.send(ChurnMsg{i}); });
    for (auto& p : publishers)
        p.join();
    publishing.store(false, std::memory_order_relaxed);
    churn.join();
    CHECK(stableCount.load() == 3 * cN);
}

// --- Lifetime during dispatch ---
// Snapshot + NoContext is rejected at compile time (compile_fail/cf_config_snapshot_no_context.cpp): without a dispatch frame a
// subscriber destroyed during a Snapshot dispatch stayed in that dispatch's snapshot (ASan: heap-use-after-free).

namespace {
using SnapStaticMsg = Msg<7, sub0::Snapshot, sub0::StaticContext, sub0::NoFilter, sub0::Capacity<8>>;
using FilterMsg     = Msg<8, sub0::Snapshot, sub0::ThreadLocalContext, sub0::Capacity<8>>; // filter enabled
struct ScopedFilterMsg { int value; using sub0_config = sub0::config<sub0::Scoped, sub0::Capacity<8>>; };

template<class Data>
struct Filtering final : sub0::Subscribe<Data>
{
    using sub0::Subscribe<Data>::Subscribe;
    std::function<void()> inFilter;
    int* received = nullptr;
    bool filter(const Data&) noexcept override
    {
        if (inFilter)
            inFilter(); // may unsubscribe or destroy *this
        return true;
    }
    void receive(const Data&) noexcept override { ++*received; }
};
} // namespace

TEST_CASE("axes: Snapshot, a later subscriber destroyed by an earlier receiver is not called (run under ASan)") {
    Source<SnapStaticMsg> pub;
    Probe<SnapStaticMsg> first;
    auto second = std::make_unique<Probe<SnapStaticMsg>>();
    first.action = [&](const SnapStaticMsg&) { second.reset(); };
    pub.send(SnapStaticMsg{1});
    CHECK(first.received == 1);
    CHECK_FALSE(second);
}

TEST_CASE("axes: filter() that unsubscribes its own subscriber prevents receive()") {
    int received = 0;
    Filtering<FilterMsg> s;
    s.received = &received;
    s.inFilter = [&] { s.unsubscribe(); };
    Source<FilterMsg> pub;
    pub.send(FilterMsg{1});
    CHECK(received == 0);
}

TEST_CASE("axes: filter() that destroys its own subscriber prevents receive() (run under ASan)") {
    int received = 0;
    auto s = std::make_unique<Filtering<FilterMsg>>();
    s->received = &received;
    s->inFilter = [&] { s.reset(); };
    Source<FilterMsg> pub;
    pub.send(FilterMsg{1});
    CHECK(received == 0);
    CHECK_FALSE(s);
}

TEST_CASE("axes: filter() that closes its domain prevents receive() for itself and later subscribers") {
    int received = 0;
    sub0::Domain<ScopedFilterMsg> domain;
    Filtering<ScopedFilterMsg> first(domain);
    Filtering<ScopedFilterMsg> second(domain);
    first.received = &received;
    second.received = &received;
    first.inFilter = [&] { domain.close(); };
    struct Pub : sub0::Publish<ScopedFilterMsg>
    {
        using Publish::Publish;
        void send(const ScopedFilterMsg& d) noexcept { sub0::publish(*this, d); }
    } pub(domain);
    pub.send(ScopedFilterMsg{1});
    CHECK(received == 0);
}

namespace {
/// A lock that, once armed on a thread, parks that thread right after its next unlock until released
struct PausingLock
{
    std::mutex mutex;
    inline static std::atomic<bool> parked{false};
    inline static std::atomic<bool> resume{false};
    inline static thread_local bool armed = false;
    void lock() noexcept { mutex.lock(); }
    void unlock() noexcept
    {
        mutex.unlock();
        if (armed)
        {
            armed = false;
            parked.store(true);
            while (!resume.load())
                std::this_thread::yield();
        }
    }
};
struct RaceMsg { int value; using sub0_config = sub0::config<sub0::Scoped, sub0::LockWith<PausingLock>>; };
struct RaceSub final : sub0::Subscribe<RaceMsg>
{
    using Subscribe::Subscribe;
    void receive(const RaceMsg&) noexcept override {}
};
} // namespace

TEST_CASE("axes: Lock, a domain closed between registration and its return leaves the subscriber detached") {
    sub0::Domain<RaceMsg> domain;
    RaceSub sub(domain);
    std::thread registering([&] {
        PausingLock::armed = true;
        CHECK(sub.trySubscribe() == sub0::SubscribeResult::Subscribed); // inserted, then parked after unlock
    });
    while (!PausingLock::parked.load())
        std::this_thread::yield();
    domain.close(); // detaches the subscriber registered a moment ago
    PausingLock::resume.store(true);
    registering.join();
    CHECK(domain.isClosed());
    CHECK_FALSE(sub.isSubscribed());
    CHECK(sub.trySubscribe() == sub0::SubscribeResult::Closed);
}
