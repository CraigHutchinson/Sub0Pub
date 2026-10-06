#pragma once
/** Case station, candidate "port member".
 *  Receivers are plain classes and the publisher is an ordinary (non-template) class holding one type-erased
 *  Sink per message type. The including variant binds the sinks (PORT_MODE) to a wire() result or to the runtime
 *  broker presented as a wiring. */
#include "collapse_case.hpp"
#include "sub0pub/sub0pub.hpp"
#include "sub0pub_spike/broker_bus.hpp"
#include "sub0pub_spike/subscription.hpp"

#define PORT_DYNAMIC 0
#define PORT_STATIC 1

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
struct Sensor {
    Sensor(sub0::Sink<Sample> s, sub0::Sink<Command> c) noexcept : samples(s), commands(c) {}
    void send(uint32_t v) noexcept
    {
        samples.publish(Sample{v});
        commands.publish(Command{v ^ 0xFU});
    }
    sub0::Sink<Sample> samples;
    sub0::Sink<Command> commands;
};
collapse::Slot<Controller> controller;
collapse::Slot<Logger> logger;
collapse::Slot<Actuator> actuator;

#if PORT_MODE == PORT_STATIC
using Bus = sub0::Wiring<Controller, Logger, Actuator>;
#else
using Bus = s0::BrokerBus;
collapse::Slot<s0::Subscription<Controller, Sample, Command>> controllerSubscription;
collapse::Slot<s0::Subscription<Logger, Sample>> loggerSubscription;
collapse::Slot<s0::Subscription<Actuator, Command>> actuatorSubscription;
#endif
collapse::Slot<Bus> bus;
collapse::Slot<Sensor> sensor;
}

COLLAPSE_ENTRY void collapse_setup()
{
    controller.emplace(); logger.emplace(); actuator.emplace();
#if PORT_MODE == PORT_STATIC
    bus.emplace(controller.get(), logger.get(), actuator.get());
#else
    bus.emplace();
    controllerSubscription.emplace(controller.get());
    loggerSubscription.emplace(logger.get());
    actuatorSubscription.emplace(actuator.get());
#endif
    sensor.emplace(sub0::Sink<Sample>(bus.get()), sub0::Sink<Command>(bus.get()));
}
COLLAPSE_ENTRY void collapse_publish(uint32_t v) { sensor->send(collapse::arg(v)); }
COLLAPSE_ENTRY void collapse_teardown()
{
    sensor.reset();
#if PORT_MODE == PORT_DYNAMIC
    actuatorSubscription.reset(); loggerSubscription.reset(); controllerSubscription.reset();
#endif
    bus.reset(); actuator.reset(); logger.reset(); controller.reset();
}
