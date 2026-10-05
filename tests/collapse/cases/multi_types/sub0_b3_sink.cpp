// COLLAPSE_REFERENCE: handwritten_erased
/** Case: two message types in one wiring. Controller handles Sample and Command, Logger only Sample,
 *  Actuator only Command. Each publication sends Sample{v} then Command{v ^ 0xF}; bound order
 *  controller, logger, actuator, so Sample reaches controller then logger, Command controller then actuator.
 *  Pattern B3: a non-template publisher holding one Sink per message type into the same typed wiring. */
#include "collapse_case.hpp"
#include "sub0pub/sub0pub.hpp"

namespace {
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
collapse::Slot<Controller> controller;
collapse::Slot<Logger> logger;
collapse::Slot<Actuator> actuator;
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
using Bus = sub0::Wiring<Controller, Logger, Actuator>;
collapse::Slot<Bus> bus;
collapse::Slot<Sensor> sensor;
}

COLLAPSE_ENTRY void collapse_setup()
{
    controller.emplace(); logger.emplace(); actuator.emplace();
    bus.emplace(controller.get(), logger.get(), actuator.get());
    sensor.emplace(sub0::Sink<Sample>(bus.get()), sub0::Sink<Command>(bus.get()));
}
COLLAPSE_ENTRY void collapse_publish(uint32_t v) { sensor->send(collapse::arg(v)); }
COLLAPSE_ENTRY void collapse_teardown() { sensor.reset(); bus.reset(); actuator.reset(); logger.reset(); controller.reset(); }
