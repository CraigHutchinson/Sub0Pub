/** The runtime broker alone: no static wiring or IPC is reachable from sub0pub/broker.hpp */
#include "sub0pub/broker.hpp"

#if defined(CROG_SUB0PUB_WIRING_HPP) || defined(CROG_SUB0PUB_WIRING_CAPABILITY_HPP)
#error "sub0pub/broker.hpp must not include static wiring"
#endif
#if defined(CROG_SUB0PUB_IPC_HPP) || defined(CROG_SUB0PUB_IPC_BINARY_WRITER_HPP)
#error "sub0pub/broker.hpp must not include IPC"
#endif

namespace {
struct BrokerOnlyMsg { int value; };
struct Out final : sub0::Publish<BrokerOnlyMsg> {};
struct In final : sub0::Subscribe<BrokerOnlyMsg>
{
    int received = 0;
    void receive(const BrokerOnlyMsg&) noexcept override { ++received; }
};
}

int useBroker()
{
    Out out;
    In in;
    sub0::publish(out, BrokerOnlyMsg{1});
    return in.received;
}
