#pragma once
/** Case one_receiver, candidate "port member".
 *  The receiver is a plain class and the publisher is an ordinary (non-template) class holding a type-erased
 *  Sink. The including variant binds the sink (PORT_MODE) to a wire() result or to the runtime broker. */
#include "collapse_case.hpp"
#include "sub0pub/sub0pub.hpp"
#include "sub0pub_spike/broker_bus.hpp"
#include "sub0pub_spike/subscription.hpp"

#define PORT_DYNAMIC 0
#define PORT_STATIC 1

namespace {
namespace s0 = sub0::spike;
struct Sample { uint32_t value; };
struct Controller {
    explicit Controller(uint32_t g) noexcept : gain(g) {}
    void receive(const Sample& s) noexcept { COLLAPSE_WORK(s.value * gain); }
    uint32_t gain;
};
struct Sensor {
    explicit Sensor(sub0::Sink<Sample> o) noexcept : out(o) {}
    void send(uint32_t v) noexcept { out.publish(Sample{v}); }
    sub0::Sink<Sample> out;
};
collapse::Slot<Controller> controller;

#if PORT_MODE == PORT_STATIC
using Bus = sub0::Wiring<Controller>;
#else
using Bus = s0::BrokerBus;
collapse::Slot<s0::Subscription<Controller, Sample>> controllerSubscription;
#endif
collapse::Slot<Bus> bus;
collapse::Slot<Sensor> sensor;
}

COLLAPSE_ENTRY void collapse_setup()
{
    controller.emplace(3U);
#if PORT_MODE == PORT_STATIC
    bus.emplace(controller.get());
#else
    bus.emplace();
    controllerSubscription.emplace(controller.get());
#endif
    sensor.emplace(sub0::Sink<Sample>(bus.get()));
}
COLLAPSE_ENTRY void collapse_publish(uint32_t v) { sensor->send(collapse::arg(v)); }
COLLAPSE_ENTRY void collapse_teardown()
{
    sensor.reset();
#if PORT_MODE == PORT_DYNAMIC
    controllerSubscription.reset();
#endif
    bus.reset(); controller.reset();
}
