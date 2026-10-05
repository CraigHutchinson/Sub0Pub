/** Case: two message types in one wiring. Controller handles Sample and Command, Logger only Sample,
 *  Actuator only Command. Each publication sends Sample{v} then Command{v ^ 0xF}; bound order
 *  controller, logger, actuator, so Sample reaches controller then logger, Command controller then actuator.
 *  Runtime-bound reference: the publisher holds the receivers' addresses (stored at setup). Equal work for B1. */
#include "collapse_case.hpp"

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
    Controller* c;
    Logger* log;
    Actuator* act;
    void send(uint32_t v) noexcept
    {
        const Sample s{v};
        c->receive(s);
        log->receive(s);
        const Command cmd{v ^ 0xFU};
        c->receive(cmd);
        act->receive(cmd);
    }
};
collapse::Slot<Sensor> sensor;
}

COLLAPSE_ENTRY void collapse_setup() { controller.emplace(); logger.emplace(); actuator.emplace(); sensor.emplace(Sensor{&controller.get(), &logger.get(), &actuator.get()}); }
COLLAPSE_ENTRY void collapse_publish(uint32_t v) { sensor->send(collapse::arg(v)); }
COLLAPSE_ENTRY void collapse_teardown() { sensor.reset(); actuator.reset(); logger.reset(); controller.reset(); }
