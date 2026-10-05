// COLLAPSE_REFERENCE: handwritten_gateway
/** Case: two independent domains, published by ONE publisher that holds both wirings (a gateway feeding two
 *  sessions): same behaviour as two publishers, one per domain. Pattern B1. */
#include "collapse_case.hpp"
#include "sub0pub/sub0pub.hpp"

namespace {
struct Sample { uint32_t value; };
struct Controller {
    explicit Controller(uint32_t g) noexcept : gain(g) {}
    void receive(const Sample& s) noexcept { COLLAPSE_WORK(s.value * gain); }
    uint32_t gain;
};
struct Logger {
    void receive(const Sample& s) noexcept { ++count; COLLAPSE_WORK(s.value ^ count); }
    uint32_t count = 0;
};
template<class OutA, class OutB>
struct Gateway {
    Gateway(const OutA& a, const OutB& b) noexcept : outA(a), outB(b) {}
    void send(uint32_t va, uint32_t vb) noexcept
    {
        outA.publish(Sample{va});
        outB.publish(Sample{vb});
    }
    OutA outA;
    OutB outB;
};
using BusA = sub0::Wiring<Controller, Logger>;
using BusB = sub0::Wiring<Controller>;
collapse::Slot<Controller> controllerA;
collapse::Slot<Logger> loggerA;
collapse::Slot<Controller> controllerB;
collapse::Slot<Gateway<BusA, BusB>> gateway;
}

COLLAPSE_ENTRY void collapse_setup()
{
    controllerA.emplace(3U); loggerA.emplace(); controllerB.emplace(5U);
    gateway.emplace(BusA(controllerA.get(), loggerA.get()), BusB(controllerB.get()));
}
COLLAPSE_ENTRY void collapse_publish(uint32_t v)
{
    const uint32_t a = collapse::arg(v);
    gateway->send(a, collapse::arg(v + 1U));
}
COLLAPSE_ENTRY void collapse_teardown() { gateway.reset(); controllerB.reset(); loggerA.reset(); controllerA.reset(); }
