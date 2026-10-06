#pragma once
/** Case station, candidate "bus parameter".
 *  Receivers are plain classes and the publisher is a template on its output. The including variant selects the
 *  bus (BUS_MODE): a StaticWiring, or the runtime broker presented as a wiring plus one Subscription per receiver. */
#include "collapse_case.hpp"
#include "sub0pub/sub0pub.hpp"
#include "sub0pub_spike/broker_bus.hpp"
#include "sub0pub_spike/subscription.hpp"

#define BUS_DYNAMIC 0
#define BUS_STATIC 1

namespace {
namespace s0 = sub0::spike;
struct Sample { uint32_t value; };
struct Command { uint32_t code; };
struct Controller {
    void receive(const Sample& s) noexcept { COLLAPSE_WORK(s.value * 3U); }
    void receive(const Command& c) noexcept { COLLAPSE_WORK(c.code + 1U); }
};
struct Logger {
    void receive(const Sample& s) noexcept { ++count; COLLAPSE_WORK(s.value ^ count); }
    uint32_t count = 0;
};
struct Actuator {
    void receive(const Command& c) noexcept { COLLAPSE_WORK(c.code << 2U); }
};
template<class Bus>
struct Sensor : sub0::Publisher<Sensor<Bus>, Bus> {
    using sub0::Publisher<Sensor<Bus>, Bus>::Publisher;
    void send(uint32_t v) noexcept
    {
        this->publish(Sample{v});
        this->publish(Command{v ^ 0xFU});
    }
};
collapse::Slot<Controller> controller;
collapse::Slot<Logger> logger;
collapse::Slot<Actuator> actuator;

#if BUS_MODE == BUS_STATIC
using Bus = sub0::StaticWiring<&controller, &logger, &actuator>;
#else
using Bus = s0::BrokerBus;
collapse::Slot<s0::Subscription<Controller, Sample, Command>> controllerSubscription;
collapse::Slot<s0::Subscription<Logger, Sample>> loggerSubscription;
collapse::Slot<s0::Subscription<Actuator, Command>> actuatorSubscription;
#endif
collapse::Slot<Sensor<Bus>> sensor;
}

COLLAPSE_ENTRY void collapse_setup()
{
    sensor.emplace(Bus{});
    controller.emplace(); logger.emplace(); actuator.emplace();
#if BUS_MODE == BUS_DYNAMIC
    controllerSubscription.emplace(controller.get());
    loggerSubscription.emplace(logger.get());
    actuatorSubscription.emplace(actuator.get());
#endif
}
COLLAPSE_ENTRY void collapse_publish(uint32_t v) { sensor->send(collapse::arg(v)); }
COLLAPSE_ENTRY void collapse_teardown()
{
#if BUS_MODE == BUS_DYNAMIC
    actuatorSubscription.reset(); loggerSubscription.reset(); controllerSubscription.reset();
#endif
    actuator.reset(); logger.reset(); controller.reset(); sensor.reset();
}
