#pragma once
/** Case one_receiver, candidate "bus parameter".
 *  The receiver is a plain class and the publisher is a template on its output. The including variant selects
 *  the bus (BUS_MODE): a StaticWiring, or the runtime broker presented as a wiring plus a Subscription. */
#include "collapse_case.hpp"
#include "sub0pub/sub0pub.hpp"
#include "sub0pub_spike/broker_bus.hpp"
#include "sub0pub_spike/subscription.hpp"

#define BUS_DYNAMIC 0
#define BUS_STATIC 1

namespace {
namespace s0 = sub0::spike;
struct Sample { uint32_t value; };
struct Controller {
    explicit Controller(uint32_t g) noexcept : gain(g) {}
    void receive(const Sample& s) noexcept { COLLAPSE_WORK(s.value * gain); }
    uint32_t gain;
};
template<class Bus>
struct Sensor : sub0::Publisher<Sensor<Bus>, Bus> {
    using sub0::Publisher<Sensor<Bus>, Bus>::Publisher;
    void send(uint32_t v) noexcept { this->publish(Sample{v}); }
};
collapse::Slot<Controller> controller;

#if BUS_MODE == BUS_STATIC
using Bus = sub0::StaticWiring<&controller>;
#else
using Bus = s0::BrokerBus;
collapse::Slot<s0::Subscription<Controller, Sample>> controllerSubscription;
#endif
collapse::Slot<Sensor<Bus>> sensor;
}

COLLAPSE_ENTRY void collapse_setup()
{
    sensor.emplace(Bus{});
    controller.emplace(3U);
#if BUS_MODE == BUS_DYNAMIC
    controllerSubscription.emplace(controller.get());
#endif
}
COLLAPSE_ENTRY void collapse_publish(uint32_t v) { sensor->send(collapse::arg(v)); }
COLLAPSE_ENTRY void collapse_teardown()
{
#if BUS_MODE == BUS_DYNAMIC
    controllerSubscription.reset();
#endif
    controller.reset(); sensor.reset();
}
