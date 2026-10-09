/** Case: two message types in one wiring. Controller handles Sample and Command, Logger only Sample,
 *  Actuator only Command. Each publication sends Sample{v} then Command{v ^ 0xF}; bound order
 *  controller, logger, actuator, so Sample reaches controller then logger, Command controller then actuator.
 *  The runtime broker at its leanest macros. Controller subscribes to both types (two bases). */
// Today's API at its leanest settings (fair comparison): direct dispatch, no assertion checks
#define SUB0PUB_ASSERT false
#include "collapse_case.hpp"
#include "sub0pub/sub0pub.hpp"

namespace {
struct Sample { uint32_t value; };
struct Command { uint32_t code; };
struct Controller final : sub0::Subscribe<Sample>, sub0::Subscribe<Command> {
    void receive(const Sample& s) noexcept override { COLLAPSE_WORK(s.value * 3U); }
    void receive(const Command& c) noexcept override { COLLAPSE_WORK(c.code + 1U); }
};
struct Logger final : sub0::Subscribe<Sample> {
    void receive(const Sample& s) noexcept override { ++count; COLLAPSE_WORK(s.value ^ count); }
    uint32_t count = 0;
};
struct Actuator final : sub0::Subscribe<Command> {
    void receive(const Command& c) noexcept override { COLLAPSE_WORK(c.code << 2U); }
};
struct Sensor : sub0::Publish<Sample>, sub0::Publish<Command> {
    void send(uint32_t v) noexcept
    {
        sub0::publish(static_cast<sub0::Publish<Sample>&>(*this), Sample{v});
        sub0::publish(static_cast<sub0::Publish<Command>&>(*this), Command{v ^ 0xFU});
    }
};
collapse::Slot<Sensor> sensor;
collapse::Slot<Controller> controller;
collapse::Slot<Logger> logger;
collapse::Slot<Actuator> actuator;
}

COLLAPSE_ENTRY void collapse_setup() { sensor.emplace(); controller.emplace(); logger.emplace(); actuator.emplace(); }
COLLAPSE_ENTRY void collapse_publish(uint32_t v) { sensor->send(collapse::arg(v)); }
COLLAPSE_ENTRY void collapse_teardown() { actuator.reset(); logger.reset(); controller.reset(); sensor.reset(); }
