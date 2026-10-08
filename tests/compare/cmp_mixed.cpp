/** Public mixed paths: identical fixed work plus 0/1/8 runtime listeners, churn and overflow.
 * Counts are dynamic listeners only: every publish also calls one fixed receiver.
 * No v1 equivalent is claimed. Use collapse evidence for hand-written, final-link comparisons.
 */
#define ANKERL_NANOBENCH_IMPLEMENT
#include "sub0pub/wiring.hpp"
#include "sub0pub/wiring/broker_port.hpp"
#include "cmp_common.hpp"
#include <array>
#include <optional>

namespace {
struct ScopedSample
{
    unsigned value;
    using sub0_config = sub0::config<sub0::Scoped>;
};
struct SnapshotSample
{
    unsigned value;
    using sub0_config = sub0::config<sub0::Scoped, sub0::Snapshot>;
};

template<class Data>
struct Fixed
{
    unsigned count = 0;
    CMP_NOINLINE void receive(const Data&) noexcept { ++count; }
};
template<class Data>
struct BrokerProbe final : sub0::Subscribe<Data>
{
    using sub0::Subscribe<Data>::Subscribe;
    unsigned count = 0;
    CMP_NOINLINE void receive(const Data&) noexcept override { ++count; }
};

template<class Bus, class Data>
CMP_NOINLINE void send(const Bus& bus, const Data& sample) noexcept { bus.publish(sample); }

template<class Data>
CMP_NOINLINE bool brokerChurn(sub0::Domain<Data>& domain) noexcept
{
    BrokerProbe<Data> probe(domain);
    ankerl::nanobench::doNotOptimizeAway(&probe);
    return probe.isSubscribed();
}

template<class Data>
void broker(bench::Harness& harness, const char* label)
{
    harness.title(label);
    sub0::Domain<Data> domain;
    Fixed<Data> fixed;
    sub0::BrokerPort<Data> port(domain);
    auto bus = sub0::wire(fixed, port);
    std::array<std::optional<BrokerProbe<Data>>, 8> probes;
    harness.run(cmp::cPublish0, [&] { send(bus, Data{42}); });
    probes[0].emplace(domain);
    harness.run(cmp::cPublish1, [&] { send(bus, Data{42}); });
    harness.run(cmp::cCreateDestroy, [&] { ankerl::nanobench::doNotOptimizeAway(brokerChurn(domain)); });
    for (unsigned i = 1; i < 8; ++i) probes[i].emplace(domain);
    harness.run(cmp::cPublish8, [&] { send(bus, Data{42}); });
    harness.run("registration rejected at capacity", [&] { ankerl::nanobench::doNotOptimizeAway(brokerChurn(domain)); });
}
}

int main()
{
    bench::Harness harness;
    broker<ScopedSample>(harness, "v2 mixed wire + BrokerPort (Scoped Direct, one fixed receiver)");
    broker<SnapshotSample>(harness, "v2 mixed wire + BrokerPort (Scoped Snapshot, one fixed receiver)");
}
