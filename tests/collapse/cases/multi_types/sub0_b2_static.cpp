/** Case: two message types in one wiring. Controller handles Sample and Command, Logger only Sample,
 *  Actuator only Command. Each publication sends Sample{v} then Command{v ^ 0xF}; bound order
 *  controller, logger, actuator, so Sample reaches controller then logger, Command controller then actuator.
 *  Pattern B2: one StaticWiring for both types; routing by capability decides, per type, at compile time. */
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
using Bus = sub0::StaticWiring<&controller, &logger, &actuator>;
template<class Out>
struct Sensor {
    void send(uint32_t v) noexcept
    {
        Out::publish(Sample{v});
        Out::publish(Command{v ^ 0xFU});
    }
};
collapse::Slot<Sensor<Bus>> sensor;
}

COLLAPSE_ENTRY void collapse_setup() { controller.emplace(); logger.emplace(); actuator.emplace(); sensor.emplace(); }
COLLAPSE_ENTRY void collapse_publish(uint32_t v) { sensor->send(collapse::arg(v)); }
COLLAPSE_ENTRY void collapse_teardown() { sensor.reset(); actuator.reset(); logger.reset(); controller.reset(); }
