// COLLAPSE_REFERENCE: handwritten_registry
/** Static/dynamic bridge on the message type, empty dynamic side: sub0::StaticFirst keeps the runtime broker
 *  reachable, but no dynamic subscriber is ever added. Checks what an unused runtime side costs. */
#include "collapse_case.hpp"
#include "sub0pub/sub0pub.hpp"

namespace {
// A receiver is named before it is defined, as an application declares `extern Controller controller;`. The
// harness constructs its objects in collapse_setup(), through a holder with the storage of collapse::Slot.
struct ControllerSlot;
struct LoggerSlot;
extern ControllerSlot controller;
extern LoggerSlot logger;
// lean registry: only the features the hand-written registry has
struct Sample { uint32_t value; using sub0_config = sub0::config<sub0::Direct, sub0::NoContext, sub0::NoFilter, sub0::StaticFirst<&controller, &logger>>; };
struct Controller final : sub0::Subscribe<Sample> {
    void receive(const Sample& s) noexcept { COLLAPSE_WORK(s.value * 3U); }
};
struct Logger final : sub0::Subscribe<Sample> {
    void receive(const Sample& s) noexcept { ++count; COLLAPSE_WORK(s.value ^ count); }
    uint32_t count = 0;
};
struct ControllerSlot : collapse::Slot<Controller> {};
struct LoggerSlot : collapse::Slot<Logger> {};
struct Sensor : sub0::Publish<Sample> {
    void send(uint32_t v) noexcept { sub0::publish(*this, Sample{v}); }
};
ControllerSlot controller;
LoggerSlot logger;
collapse::Slot<Sensor> sensor;
}

COLLAPSE_ENTRY void collapse_setup() { controller.emplace(); logger.emplace(); sensor.emplace(); }
COLLAPSE_ENTRY void collapse_publish(uint32_t v) { sensor->send(collapse::arg(v)); }
COLLAPSE_ENTRY void collapse_teardown() { sensor.reset(); logger.reset(); controller.reset(); }
