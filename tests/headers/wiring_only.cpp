/** Static wiring alone: no configuration, runtime broker or IPC is reachable from sub0pub/wiring.hpp */
#include "sub0pub/wiring.hpp"

#if defined(CROG_SUB0PUB_CONFIG_HPP) || defined(CROG_SUB0PUB_BROKER_HPP) || defined(CROG_SUB0PUB_BROKER_TABLE_HPP) \
    || defined(CROG_SUB0PUB_BROKER_SUBSCRIBE_HPP) || defined(CROG_SUB0PUB_BROKER_PUBLISH_HPP)
#error "sub0pub/wiring.hpp must not include the runtime broker or its configuration"
#endif
#if defined(CROG_SUB0PUB_IPC_HPP) || defined(CROG_SUB0PUB_IPC_BINARY_WRITER_HPP) || defined(CROG_SUB0PUB_UTILITY_STREAMS_HPP)
#error "sub0pub/wiring.hpp must not include IPC or streams"
#endif

namespace {
struct Sample { int value; };
struct Counter { int total = 0; void receive(const Sample& s) noexcept { total += s.value; } };
struct Late final : sub0::DynamicPort<Sample>::Receiver
{
    int total = 0;
    void receive(const Sample& s) noexcept override { total += s.value; }
};
}

int useWiring()
{
    Counter a, b;
    Late late;
    sub0::DynamicPort<Sample> port;
    port.add(&late);
    auto bus = sub0::wire(a, b, port);
    bus.publish(Sample{1});
    return a.total + b.total + late.total;
}
