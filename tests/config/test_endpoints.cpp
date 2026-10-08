/** Worked example of the runtime broker's endpoints:
 *   two isolated sessions and the same message type, two transport implementations, ingress and egress,
 *   transport rejection, teardown during delivery (same thread and cross-thread), and an application-defined
 *   broker implementation. Everything below is application code except the public header.
 */
#include "doctest.h"
#include "app_types.hpp"

#include <array>
#include <atomic>
#include <memory>
#include <optional>
#include <thread>
#include <vector>

// ============================================================================
// Application message types
// ============================================================================

/// One telemetry channel; each session (Domain) gets independent routing for it
struct Telemetry
{
    int seq;
    int value;
    using sub0_config = sub0::config<sub0::Scoped>;
};

// ============================================================================
// Application transports (Transport concept: SendResult send(const Data&) noexcept)
// ============================================================================

/// Synchronous in-process "cable": hands each message straight to the peer route's ingress
template<class Data>
class PipeTransport
{
public:
    using PeerRoute = sub0::Route<Data, PipeTransport<Data>>;

    void connect(const PeerRoute* peer) noexcept { peer_ = peer; }
    void close() noexcept { closed_ = true; }

    sub0::SendResult send(const Data& data) noexcept
    {
        ++sends;
        if (closed_)
            return sub0::SendResult::Closed;
        if (peer_ == nullptr)
            return sub0::SendResult::Disconnected;
        const Data copy = data; // accept by copy: never keep a reference to the publisher's object
        peer_->inject(copy);
        return sub0::SendResult::Accepted;
    }

    int sends = 0;

private:
    const PeerRoute* peer_ = nullptr;
    bool closed_ = false;
};

/// Asynchronous bounded queue (e.g. a radio or IPC ring buffer): copies at acceptance, delivered later by pump()
template<class Data, std::size_t N>
class QueueTransport
{
public:
    sub0::SendResult send(const Data& data) noexcept
    {
        if (closed_)
            return sub0::SendResult::Closed;
        if (size_ == N)
            return sub0::SendResult::Full;
        ring_[(head_ + size_++) % N] = data;
        return sub0::SendResult::Accepted;
    }

    void close() noexcept { closed_ = true; }

    /// The remote side: deliver queued messages into another session through its ingress route
    template<class Route>
    std::size_t pump(const Route& ingress) noexcept
    {
        std::size_t n = 0;
        while (size_ > 0)
        {
            const Data data = ring_[head_];
            head_ = (head_ + 1) % N;
            --size_;
            ingress.inject(data);
            ++n;
        }
        return n;
    }

private:
    std::array<Data, N> ring_{};
    std::size_t head_ = 0;
    std::size_t size_ = 0;
    bool closed_ = false;
};

/// Ingress-only placeholder transport for a route that only receives
template<class Data>
struct NullTransport
{
    sub0::SendResult send(const Data&) noexcept { ++sends; return sub0::SendResult::Accepted; }
    int sends = 0;
};

// ============================================================================
// Application subscribers and publishers
// ============================================================================

namespace {

template<class Data>
struct Recorder final : sub0::Subscribe<Data>
{
    using sub0::Subscribe<Data>::Subscribe;
    std::vector<int> seqs;
    void receive(const Data& d) noexcept override { seqs.push_back(d.seq); }
};

template<class Data>
struct Source : sub0::Publish<Data>
{
    using sub0::Publish<Data>::Publish;
    void send(const Data& d) noexcept { sub0::publish(*this, d); }
    void send(const Data& d, sub0::PublishReport& report) noexcept { sub0::publish(*this, d, report); }
};

using Pipe = PipeTransport<Telemetry>;
using PipeRoute = sub0::Route<Telemetry, Pipe>;

} // namespace

// ============================================================================
// Worked example
// ============================================================================

TEST_CASE("endpoints: two sessions, same type, isolated") {
    sub0::Domain<Telemetry> a, b;
    Recorder<Telemetry> inA(a), inB(b);
    Source<Telemetry> pubA(a);

    pubA.send(Telemetry{1, 10});
    CHECK(inA.seqs == std::vector<int>{1});
    CHECK(inB.seqs.empty());
}

TEST_CASE("endpoints: bidirectional ingress/egress between sessions, split horizon prevents echo") {
    sub0::Domain<Telemetry> a, b;
    Pipe toB, toA;
    PipeRoute routeA(a, toB); // A egress -> B ingress
    PipeRoute routeB(b, toA); // B egress -> A ingress
    toB.connect(&routeB);
    toA.connect(&routeA);

    Recorder<Telemetry> inA(a), inB(b);
    Source<Telemetry> pubA(a), pubB(b);

    pubA.send(Telemetry{1, 0});
    CHECK(inA.seqs == std::vector<int>{1});
    CHECK(inB.seqs == std::vector<int>{1});
    CHECK(toB.sends == 1);
    CHECK(toA.sends == 0); // routeB did not send A's message back (split horizon)

    pubB.send(Telemetry{2, 0});
    CHECK(inA.seqs == std::vector<int>{1, 2});
    CHECK(inB.seqs == std::vector<int>{1, 2});
    CHECK(toA.sends == 1);
    CHECK(toB.sends == 1);
}

TEST_CASE("endpoints: two transports, rejection is reported, local delivery continues") {
    sub0::Domain<Telemetry> a, c;
    Pipe pipe;                                       // disconnected: no peer yet
    QueueTransport<Telemetry, 2> queue;             // bounded async uplink
    PipeRoute pipeRoute(a, pipe);
    sub0::Route<Telemetry, QueueTransport<Telemetry, 2>> queueRoute(a, queue);
    Recorder<Telemetry> local(a);
    Source<Telemetry> pub(a);

    sub0::PublishReport r1, r2, r3;
    pub.send(Telemetry{1, 0}, r1);
    pub.send(Telemetry{2, 0}, r2);
    pub.send(Telemetry{3, 0}, r3);

    CHECK(local.seqs == std::vector<int>{1, 2, 3});   // unaffected by any route result

    CHECK(r1.routed == 2);
    CHECK(r1.accepted == 1);                          // queue accepted
    CHECK(r1.rejected == 1);
    CHECK(r1.lastRejection == sub0::SendResult::Disconnected);

    CHECK(r3.accepted == 0);                          // queue full, pipe disconnected
    CHECK(r3.rejected == 2);
    CHECK(r3.lastRejection == sub0::SendResult::Full);

    // Remote side drains the queue into another session: payloads were copied at acceptance
    NullTransport<Telemetry> none;
    sub0::Route<Telemetry, NullTransport<Telemetry>> ingressC(c, none);
    Recorder<Telemetry> remote(c);
    CHECK(queue.pump(ingressC) == 2);
    CHECK(remote.seqs == std::vector<int>{1, 2});
    CHECK(none.sends == 0);                           // ingress route does not echo its own messages

    queue.close();
    sub0::PublishReport r4;
    pub.send(Telemetry{4, 0}, r4);
    CHECK(r4.rejected == 2);                          // pipe Disconnected, then queue Closed
    CHECK(r4.lastRejection == sub0::SendResult::Closed);
}

TEST_CASE("endpoints: teardown during delivery on the same thread") {
    sub0::Domain<Telemetry> a;
    Pipe pipe;
    std::optional<PipeRoute> route;
    std::optional<Recorder<Telemetry>> victim;

    struct Killer : sub0::Subscribe<Telemetry> {
        using sub0::Subscribe<Telemetry>::Subscribe;
        std::optional<PipeRoute>* route = nullptr;
        std::optional<Recorder<Telemetry>>* victim = nullptr;
        void receive(const Telemetry&) noexcept override { route->reset(); victim->reset(); }
    };
    Killer killer(a);                 // first in the snapshot
    killer.route = &route;
    killer.victim = &victim;
    route.emplace(a, pipe);           // later in the snapshot
    victim.emplace(a);

    Recorder<Telemetry> survivor(a);
    Source<Telemetry> pub(a);
    pub.send(Telemetry{1, 0});

    CHECK_FALSE(route.has_value());
    CHECK_FALSE(victim.has_value());
    CHECK(pipe.sends == 0);                       // destroyed route's transport was never called
    CHECK(survivor.seqs == std::vector<int>{1});  // the rest of the dispatch continued

    struct SelfDisconnect : sub0::Subscribe<Telemetry> {
        using sub0::Subscribe<Telemetry>::Subscribe;
        int received = 0;
        void receive(const Telemetry&) noexcept override { ++received; disconnect(); }
    };
    SelfDisconnect once(a);
    pub.send(Telemetry{2, 0});
    pub.send(Telemetry{3, 0});
    CHECK(once.received == 1);
    CHECK_FALSE(once.isSubscribed());
}

TEST_CASE("endpoints: domain close ends the session") {
    sub0::Domain<Telemetry> a;
    Recorder<Telemetry> sub(a);
    Source<Telemetry> pub(a);
    pub.send(Telemetry{1, 0});

    a.close();
    CHECK(a.isClosed());
    CHECK_FALSE(sub.isSubscribed());
    pub.send(Telemetry{2, 0});                                   // dropped
    CHECK(sub.seqs == std::vector<int>{1});
    CHECK(sub.trySubscribe() == sub0::SubscribeResult::Closed);
}

// ----------------------------------------------------------------------------
// Cross-thread teardown (run under ASan/UBSan and TSan in CI)
// ----------------------------------------------------------------------------

namespace {

struct SpinLock
{
    std::atomic_flag flag = ATOMIC_FLAG_INIT;
    void lock() noexcept { while (flag.test_and_set(std::memory_order_acquire)) std::this_thread::yield(); }
    void unlock() noexcept { flag.clear(std::memory_order_release); }
};

} // namespace

struct Shared
{
    int seq;
    /// Its subscribers are created and destroyed while another thread publishes: the table is empty in between
    using sub0_config = sub0::config<sub0::Scoped, sub0::LockWith<SpinLock>, sub0::AllowNoReceivers>;
};

namespace {

/// Follows the contract: activates at the end of its constructor, disconnects first in its destructor.
/// Its receive() writes through a heap pointer freed right after disconnect(): a late call is a use-after-free.
struct Guarded final : sub0::Subscribe<Shared>
{
    explicit Guarded(sub0::Domain<Shared>& d) : sub0::Subscribe<Shared>(d), counter(new int(0)) { trySubscribe(); }
    ~Guarded()
    {
        disconnect();
        delete counter;
        counter = nullptr;
    }
    void receive(const Shared&) noexcept override
    {
        for (volatile int spin = 0; spin < 200; ++spin) {} // widen the window a late call would hit
        ++*counter;
    }
    int* counter;
};

} // namespace

TEST_CASE("endpoints: teardown during delivery from another thread") {
    sub0::Domain<Shared> domain;
    std::atomic<bool> stop{false};
    std::atomic<int> published{0};

    std::thread publisher([&] {
        Source<Shared> pub(domain);
        while (!stop.load(std::memory_order_relaxed))
        {
            pub.send(Shared{published.fetch_add(1, std::memory_order_relaxed)});
        }
    });

    // Teardown must overlap publishing: without this wait a starved publisher thread (loaded CI runner) might
    // not publish at all before stop, leaving the race untested and the final CHECK failing
    while (published.load(std::memory_order_relaxed) == 0)
        std::this_thread::yield();

    for (int i = 0; i < 2000; ++i)
    {
        Guarded a(domain);
        auto b = std::make_unique<Guarded>(domain);
        std::this_thread::yield();
    }

    // End the session while the publisher is still running, then stop it
    domain.close();
    stop.store(true, std::memory_order_relaxed);
    publisher.join();
    CHECK(published.load() > 0);
}

// ============================================================================
// Application-defined broker implementation (Implementation<> hook), built only on sub0::kit
// ============================================================================

namespace {

/// A broker for channels with at most one subscriber: no table scan, one pointer of RAM, one indirect call
template<class Data, class Config>
class SingleSubscriberBroker
{
public:
    SingleSubscriberBroker() noexcept = default;

    sub0::SubscribeResult trySubscribe(sub0::Subscribe<Data>* s) noexcept
    {
        if (slot() != nullptr)
            return sub0::SubscribeResult::CapacityExceeded;
        slot() = s;
        return sub0::SubscribeResult::Subscribed;
    }

    void disconnect(sub0::Subscribe<Data>* s) noexcept
    {
        if (slot() == s)
            slot() = nullptr;
        sub0::kit::forgetInOwnDispatches<Data>(&slot(), s);
    }

    void publish(const Data& data, const void* origin, sub0::PublishReport* report) const noexcept
    {
        sub0::Subscribe<Data>* snapshot[1] = { slot() };
        sub0::kit::DispatchScope<Data> scope(&slot(), origin, report, snapshot, 1);
        sub0::kit::deliverAt<Data>(snapshot[0], data);
    }

    void cancel() const noexcept { sub0::kit::cancel<Data>(&slot()); }

private:
    static sub0::Subscribe<Data>*& slot() noexcept
    {
        static sub0::Subscribe<Data>* s = nullptr;
        return s;
    }
};

} // namespace

struct Status
{
    int seq;
    using sub0_config = sub0::config<sub0::Implementation<SingleSubscriberBroker>>;
};

TEST_CASE("endpoints: application-defined broker through Implementation<>, with a route") {
    Recorder<Status> only;
    Recorder<Status> second;
    CHECK(only.isSubscribed());
    CHECK_FALSE(second.isSubscribed());   // this broker's own capacity rule

    Source<Status> pub;
    pub.send(Status{7});
    CHECK(only.seqs == std::vector<int>{7});

    only.disconnect();
    CHECK(second.trySubscribe() == sub0::SubscribeResult::Subscribed);

    // Routes work with any broker that pushes kit::DispatchScope frames
    NullTransport<Status> uplink;
    second.disconnect();
    sub0::Route<Status, NullTransport<Status>> route(uplink);
    sub0::PublishReport report;
    pub.send(Status{8}, report);
    CHECK(uplink.sends == 1);
    CHECK(report.accepted == 1);
}
