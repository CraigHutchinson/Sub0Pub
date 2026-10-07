/** Case: two message types in one wiring. Controller handles Sample and Command, Logger only Sample,
 *  Actuator only Command. Each publication sends Sample{v} then Command{v ^ 0xF}; Sample reaches controller then
 *  logger, Command controller then actuator.
 *  The typed topology: each message type names its own receivers (sub0::StaticTo); one receiver subscribes to both. */
#include "collapse_case.hpp"
#include "sub0pub/sub0pub.hpp"

namespace {
// A receiver is named before it is defined, as an application declares `extern Controller controller;`. The
// harness constructs its objects in collapse_setup(), through a holder with the storage of collapse::Slot.
struct ControllerSlot;
struct LoggerSlot;
struct ActuatorSlot;
extern ControllerSlot controller;
extern LoggerSlot logger;
extern ActuatorSlot actuator;
struct Sample { uint32_t value; using sub0_config = sub0::config<sub0::StaticTo<&controller, &logger>>; };
struct Command { uint32_t code; using sub0_config = sub0::config<sub0::StaticTo<&controller, &actuator>>; };
struct Controller final : sub0::SubscribeAll<Sample, Command> {
    void receive(const Sample& s) noexcept { COLLAPSE_WORK(s.value * 3U); }
    void receive(const Command& c) noexcept { COLLAPSE_WORK(c.code + 1U); }
};
struct Logger final : sub0::Subscribe<Sample> {
    void receive(const Sample& s) noexcept { ++count; COLLAPSE_WORK(s.value ^ count); }
    uint32_t count = 0;
};
struct Actuator final : sub0::Subscribe<Command> {
    void receive(const Command& c) noexcept { COLLAPSE_WORK(c.code << 2U); }
};
struct ControllerSlot : collapse::Slot<Controller> {};
struct LoggerSlot : collapse::Slot<Logger> {};
struct ActuatorSlot : collapse::Slot<Actuator> {};
struct SUB0PUB_EMPTY_BASES Sensor : sub0::Publish<Sample>, sub0::Publish<Command> {
    void send(uint32_t v) noexcept
    {
        sub0::publish(*this, Sample{v});
        sub0::publish(*this, Command{v ^ 0xFU});
    }
};
ControllerSlot controller;
LoggerSlot logger;
ActuatorSlot actuator;
collapse::Slot<Sensor> sensor;
}

COLLAPSE_ENTRY void collapse_setup() { controller.emplace(); logger.emplace(); actuator.emplace(); sensor.emplace(); }
COLLAPSE_ENTRY void collapse_publish(uint32_t v) { sensor->send(collapse::arg(v)); }
COLLAPSE_ENTRY void collapse_teardown() { sensor.reset(); actuator.reset(); logger.reset(); controller.reset(); }
