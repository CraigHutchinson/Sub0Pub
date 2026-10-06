/** Case: two message types in one wiring. Controller handles Sample and Command, Logger only Sample,
 *  Actuator only Command. Each publication sends Sample{v} then Command{v ^ 0xF}; bound order
 *  controller, logger, actuator, so Sample reaches controller then logger, Command controller then actuator.
 *  Reference: direct calls. */
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
}

COLLAPSE_ENTRY void collapse_setup() { controller.emplace(); logger.emplace(); actuator.emplace(); }
COLLAPSE_ENTRY void collapse_publish(uint32_t v)
{
    const Sample s{collapse::arg(v)};
    controller->receive(s);
    logger->receive(s);
    const Command c{s.value ^ 0xFU};
    controller->receive(c);
    actuator->receive(c);
}
COLLAPSE_ENTRY void collapse_teardown() { actuator.reset(); logger.reset(); controller.reset(); }
