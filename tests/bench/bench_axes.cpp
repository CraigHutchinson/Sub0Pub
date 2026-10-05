/** Every runtime-broker configuration axis, one option at a time (docs/DESIGN.md, "Per-type configuration")
 *
 * Each option is varied alone from two bases, so its cost is attributable to that option only:
 *   - Full: Snapshot dispatch, thread-local publish context and filter() (the costliest valid base without a lock);
 *   - Lean: Direct, NoContext, NoFilter (the library default: the cheapest valid configuration).
 * Each configuration is bound to its own message type, so all of them run in one binary. Same control conditions as
 * bench_core.cpp: out-of-line publish and lifetime entry points, out-of-line receivers.
 *
 * Scenarios (instr/op under callgrind, ns/op natively): publish to 0, 1 and 8 subscribers; create + destroy a
 * subscriber; a re-entrant publish (a receiver publishes the same type once, where the dispatch policy allows it);
 * and, natively only, publish from 4 threads at once for the lock options (contention).
 */
#define ANKERL_NANOBENCH_IMPLEMENT
#include "sub0pub/broker.hpp"

#include "bench_harness.hpp"
#include "bench_system_info.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#if defined(_MSC_VER)
#define BENCH_NOINLINE __declspec(noinline)
#else
#define BENCH_NOINLINE __attribute__((noinline))
#endif

namespace bench_axes {

struct SpinLock
{
    std::atomic_flag flag = ATOMIC_FLAG_INIT;
    void lock() noexcept { while (flag.test_and_set(std::memory_order_acquire)) std::this_thread::yield(); }
    void unlock() noexcept { flag.clear(std::memory_order_release); }
};

struct MutexLock
{
    std::mutex m;
    void lock() noexcept { m.lock(); }
    void unlock() noexcept { m.unlock(); }
};

/// The Implementation<> hook's worked example (test_endpoints.cpp): one subscriber, one pointer of RAM
template<class Data, class Config>
class SingleSubscriberBroker
{
public:
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
    static sub0::Subscribe<Data>*& slot() noexcept { static sub0::Subscribe<Data>* s = nullptr; return s; }
};

/// A transport that accepts everything (route egress cost without transport work)
template<class Data>
struct NullTransport
{
    sub0::SendResult send(const Data&) noexcept { return sub0::SendResult::Accepted; }
};

/// The Full base: every option below is applied on top of it, one at a time
using Full = sub0::config<sub0::Snapshot, sub0::ThreadLocalContext, sub0::Filter>;

template<int Id, class... Opts>
struct Msg
{
    int v;
    using sub0_config = sub0::config<Opts...>;
};

template<int Id, class... Opts>
struct FullMsg
{
    int v;
    using sub0_config = sub0::with<Full, Opts...>;
};

using namespace sub0;
// Full base, one option changed at a time
using D          = FullMsg<0>;
using D_Direct   = FullMsg<1, Direct>;
using D_Checked  = FullMsg<2, DirectChecked>;
using D_Static   = FullMsg<3, StaticContext>;
// Context=None from Full is invalid: Snapshot needs a publish context (tests/config/compile_fail/cf_config_snapshot_no_context.cpp)
using D_NoFilter = FullMsg<5, NoFilter>;
using D_Spin     = FullMsg<6, LockWith<SpinLock>>;
using D_Mutex    = FullMsg<7, LockWith<MutexLock>>;
using D_Scoped   = FullMsg<8, Scoped>;
using D_Cap64    = FullMsg<9, Capacity<64>>;
using D_Impl     = FullMsg<10, Implementation<SingleSubscriberBroker>>;
using D_Route    = FullMsg<11>; // same configuration as D; used with a Route bound
// Lean base, one option changed at a time
using L          = Msg<20, Direct, NoContext, NoFilter>;
using L_Snapshot = Msg<21, Snapshot, StaticContext, NoFilter>; // Snapshot needs a context (cheapest: Static)
using L_Checked  = Msg<22, DirectChecked, StaticContext, NoFilter>; // DirectChecked needs a context: the cheaper one
using L_Static   = Msg<23, Direct, StaticContext, NoFilter>;
using L_TLS      = Msg<24, Direct, ThreadLocalContext, NoFilter>;
using L_Filter   = Msg<25, Direct, NoContext>;
using L_Scoped   = Msg<26, Direct, NoContext, NoFilter, Scoped>;
using L_Cap64    = Msg<27, Direct, NoContext, NoFilter, Capacity<64>>;

template<class Data> constexpr bool cScoped = config_t<Data>::storage == Storage::Scoped;
template<class Data> constexpr bool cConcurrentT = detail::cConcurrent<config_t<Data>>;

struct NoDomain {};
template<class Data> using DomainFor = std::conditional_t<cScoped<Data>, Domain<Data>, NoDomain>;

template<class Data>
struct NoOpSink final : Subscribe<Data>
{
    NoOpSink() noexcept { activate(); }
    explicit NoOpSink(Domain<Data>& d) noexcept : Subscribe<Data>(d) { activate(); }
    // Lifetime contract (docs/DESIGN.md, "Contracts"): disconnect first in the most-derived destructor where
    // publishers may run on other threads; single-threaded configurations rely on the base destructor alone
    ~NoOpSink() { if constexpr (cConcurrentT<Data>) this->disconnect(); }
    void receive(const Data&) noexcept override;
private:
    void activate() noexcept { if constexpr (cConcurrentT<Data>) this->trySubscribe(); }
};
template<class Data> void NoOpSink<Data>::receive(const Data&) noexcept {}

template<class Data>
struct Source : Publish<Data>
{
    Source() noexcept = default;
    explicit Source(Domain<Data>& d) noexcept : Publish<Data>(d) {}
    BENCH_NOINLINE void send(const Data& d) noexcept { sub0::publish(*this, d); }
};

/// Publishes the same type once from inside its own receive() (depth 1)
template<class Data>
struct Echo final : Subscribe<Data>
{
    explicit Echo(Source<Data>& s) noexcept : src(s) { activate(); }
    Echo(Domain<Data>& d, Source<Data>& s) noexcept : Subscribe<Data>(d), src(s) { activate(); }
    ~Echo() { if constexpr (cConcurrentT<Data>) this->disconnect(); }
    void receive(const Data& d) noexcept override;
    Source<Data>& src;
    bool inside = false;
private:
    void activate() noexcept { if constexpr (cConcurrentT<Data>) this->trySubscribe(); }
};
template<class Data> void Echo<Data>::receive(const Data& d) noexcept
{
    if (inside)
        return;
    inside = true;
    src.send(d);
    inside = false;
}

// Two receivers per type (NoOpSink, Echo), defined out of line: dispatch stays a genuine virtual call, as in
// bench_core.cpp. (No explicit instantiation: it would instantiate the Global-only and Scoped-only constructors.)

/// Construction helpers that pass the domain only where the type is Scoped
template<class Data, class T, class... A>
std::unique_ptr<T> make(DomainFor<Data>& domain, A&... a)
{
    if constexpr (cScoped<Data>)
        return std::make_unique<T>(domain, a...);
    else
    {
        (void)domain;
        return std::make_unique<T>(a...);
    }
}

template<class Data>
BENCH_NOINLINE void createDestroy(DomainFor<Data>& domain) noexcept
{
    if constexpr (cScoped<Data>)
    {
        NoOpSink<Data> sub(domain);
        ankerl::nanobench::doNotOptimizeAway(&sub);
    }
    else
    {
        (void)domain;
        NoOpSink<Data> sub;
        ankerl::nanobench::doNotOptimizeAway(&sub);
    }
}

template<class Data>
void publishN(bench::Harness& h, DomainFor<Data>& domain, std::size_t n, const std::string& name)
{
    auto pub = make<Data, Source<Data>>(domain);
    std::vector<std::unique_ptr<NoOpSink<Data>>> subs;
    for (std::size_t i = 0; i < n; ++i)
        subs.push_back(make<Data, NoOpSink<Data>>(domain));
    h.run(name, [&] { pub->send(Data{42}); });
}

/// Scenarios every configuration runs; capacity-1 implementations skip 8 subscribers, and a dispatch that
/// rejects re-entry (DirectChecked) skips the re-entrant publish
template<class Data>
void runAxis(bench::Harness& h, const std::string& label, bool eight = true, bool reentrant = true)
{
    h.title(label);
    DomainFor<Data> domain;
    publishN<Data>(h, domain, 0, "publish, 0 subscribers");
    publishN<Data>(h, domain, 1, "publish, 1 subscriber");
    if (eight)
        publishN<Data>(h, domain, 8, "publish, 8 subscribers");
    h.run("create + destroy subscriber", [&] { createDestroy<Data>(domain); });
    if (reentrant)
    {
        auto pub = make<Data, Source<Data>>(domain);
        auto echo = make<Data, Echo<Data>>(domain, *pub);
        h.run("re-entrant publish (depth 1)", [&] { pub->send(Data{42}); });
    }
}

/// A Route (transport endpoint) bound to the type: its egress cost, alone and next to one local subscriber
template<class Data>
void runRoute(bench::Harness& h, const std::string& label)
{
    h.title(label);
    NullTransport<Data> transport;
    Source<Data> pub;
    {
        Route<Data, NullTransport<Data>> route(transport);
        h.run("publish, 1 route", [&] { pub.send(Data{42}); });
        NoOpSink<Data> sub;
        h.run("publish, 1 subscriber + 1 route", [&] { pub.send(Data{42}); });
    }
}

/// Contention (native timing only; callgrind serialises threads): 4 threads publish to 1 subscriber at once
template<class Data>
void contended(const std::string& label, unsigned threads)
{
    NoOpSink<Data> sub;
    constexpr int cPerThread = 200000;
    std::atomic<bool> go{false};
    std::vector<std::thread> pool;
    for (unsigned t = 0; t < threads; ++t)
        pool.emplace_back([&] {
            Source<Data> pub;
            while (!go.load(std::memory_order_acquire)) {}
            for (int i = 0; i < cPerThread; ++i)
                pub.send(Data{i});
        });
    const auto start = std::chrono::steady_clock::now();
    go.store(true, std::memory_order_release);
    for (auto& t : pool)
        t.join();
    const double ns = std::chrono::duration<double, std::nano>(std::chrono::steady_clock::now() - start).count();
    std::cout << "| " << label << " | " << threads << " threads | " << ns / (double(cPerThread) * threads)
              << " ns/publish (wall time / total publishes) |" << std::endl;
}

} // namespace bench_axes

int main()
{
    using namespace bench_axes;
    printSystemInfo();
    std::cout << "Runtime broker configuration axes, one option at a time (docs/DESIGN.md)" << std::endl << std::endl;

    bench::Harness h;
    runAxis<D>(h, "D Full (Snapshot, ThreadLocal, filter, no lock, Global, capacity 8)");
    runAxis<D_Direct>(h, "D Dispatch=Direct");
    runAxis<D_Checked>(h, "D Dispatch=DirectChecked", true, false);
    runAxis<D_Static>(h, "D Context=Static");
    runAxis<D_NoFilter>(h, "D Filter=off");
    runAxis<D_Spin>(h, "D Lock=spin");
    runAxis<D_Mutex>(h, "D Lock=std::mutex");
    runAxis<D_Scoped>(h, "D Storage=Scoped");
    runAxis<D_Cap64>(h, "D Capacity=64");
    runAxis<D_Impl>(h, "D Implementation=SingleSubscriberBroker", false);
    runRoute<D_Route>(h, "D Route=1 (NullTransport)");

    runAxis<L>(h, "L Lean (Direct, NoContext, NoFilter, no lock, Global, capacity 8)");
    runAxis<L_Snapshot>(h, "L Dispatch=Snapshot (+StaticContext, required)");
    runAxis<L_Checked>(h, "L Dispatch=DirectChecked (+StaticContext, required)", true, false);
    runAxis<L_Static>(h, "L Context=Static");
    runAxis<L_TLS>(h, "L Context=ThreadLocal");
    runAxis<L_Filter>(h, "L Filter=on");
    runAxis<L_Scoped>(h, "L Storage=Scoped");
    runAxis<L_Cap64>(h, "L Capacity=64");

    if (!h.counting())
    {
        std::cout << std::endl << "Contention (native only)" << std::endl;
        const unsigned n = std::max(2u, std::min(4u, std::thread::hardware_concurrency()));
        contended<D_Spin>("D Lock=spin", n);
        contended<D_Mutex>("D Lock=std::mutex", n);
    }
    return 0;
}
