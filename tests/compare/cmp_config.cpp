/** v1 vs v2 comparison: the v2 per-type configuration (sub0pub/config.hpp).
 *
 * One message type per configuration, all in one binary. Scenarios a configuration does not offer
 * (filter() without the filter option, cancel() without a publish context) are left out.
 */
#define ANKERL_NANOBENCH_IMPLEMENT
#include "sub0pub/sub0pub.hpp"

#include "cmp_common.hpp"

#include <atomic>
#include <string>

struct SpinLock
{
    std::atomic_flag flag = ATOMIC_FLAG_INIT;
    void lock() noexcept { while (flag.test_and_set(std::memory_order_acquire)) {} }
    void unlock() noexcept { flag.clear(std::memory_order_release); }
};

struct MsgDefault { int v; }; // Builtin: Direct, no context, no filter (the default)
struct MsgSnapStatic { int v; using sub0_config = sub0::config<sub0::Snapshot, sub0::StaticContext, sub0::Filter>; };
struct MsgFull { int v; using sub0_config = sub0::config<sub0::Snapshot, sub0::ThreadLocalContext, sub0::Filter>; };
struct MsgLean { int v; using sub0_config = sub0::config<sub0::Direct, sub0::NoContext, sub0::NoFilter>; };
struct MsgLocked { int v; using sub0_config = sub0::config<sub0::LockWith<SpinLock>, sub0::Filter>; };

namespace cmp_types {
// Every receiver does the same observable work (one increment), as in cmp_sub0pub.cpp
template<class Data>
struct Counting : sub0::Subscribe<Data> {
    Counting() noexcept { if constexpr (sub0::detail::cConcurrent<sub0::config_t<Data>>) this->trySubscribe(); }
    int count = 0;
    void receive(const Data&) noexcept override;
};
template<class Data>
struct NoOp : sub0::Subscribe<Data> { // second implementation: receive() stays a real virtual call
    void receive(const Data&) noexcept override;
};
template<class Data>
struct Filtered : Counting<Data> {
    bool filter(const Data& d) noexcept override;
};
template<class Data>
struct Cancelling : Counting<Data> {
    void receive(const Data&) noexcept override;
};
template<class Data> void NoOp<Data>::receive(const Data&) noexcept {}
template<class Data> void Counting<Data>::receive(const Data&) noexcept { ++count; }
template<class Data> bool Filtered<Data>::filter(const Data& d) noexcept { return (d.v & 1) == 0; }
template<class Data> void Cancelling<Data>::receive(const Data&) noexcept { ++this->count; this->cancel(); }

template<class Data>
struct Source : sub0::Publish<Data> {
    CMP_NOINLINE void send(const Data& d) noexcept { sub0::publish(*this, d); }
};

template<class Data>
CMP_NOINLINE void createDestroy() noexcept
{
    Counting<Data> sub;
    ankerl::nanobench::doNotOptimizeAway(&sub);
}

template<class Data>
void runConfig(bench::Harness& h, const std::string& label)
{
    using Config = sub0::config_t<Data>;
    h.title(label);
    {
        Source<Data> pub;
        h.run(cmp::cPublish0, [&] { pub.send(Data{42}); });
    }
    {
        Source<Data> pub;
        Counting<Data> sub;
        h.run(cmp::cPublish1, [&] { pub.send(Data{42}); });
    }
    {
        Source<Data> pub;
        Counting<Data> subs[8];
        (void)subs;
        h.run(cmp::cPublish8, [&] { pub.send(Data{42}); });
    }
    if constexpr (Config::filter)
    {
        Source<Data> pub;
        Filtered<Data> sub;
        h.run(cmp::cFiltered, [&] { pub.send(Data{42}); });
    }
    if constexpr (Config::context != sub0::Context::None)
    {
        Source<Data> pub;
        Cancelling<Data> first;
        Counting<Data> rest[7];
        (void)rest;
        h.run(cmp::cCancel8, [&] { pub.send(Data{42}); });
    }
    h.run(cmp::cCreateDestroy, [&] { createDestroy<Data>(); });
    NoOp<Data> unused; // keeps a second override live
    (void)unused;
}
} // namespace cmp_types

int main()
{
    using namespace cmp_types;
    bench::Harness h;
    runConfig<MsgDefault>(h, "v2 config default (Direct, no context, no filter)");
    runConfig<MsgFull>(h, "v2 config Full (Snapshot, ThreadLocal, filter)");
    runConfig<MsgSnapStatic>(h, "v2 config Full with StaticContext (no TLS)");
    runConfig<MsgLean>(h, "v2 config Lean (Direct, NoContext, NoFilter)");
    runConfig<MsgLocked>(h, "v2 config Locked (spin lock, filter, uncontended)");
    return 0;
}
