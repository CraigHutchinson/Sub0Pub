/** Static/dynamic bridge on the message type: sub0::StaticFirst calls the controller and the logger directly, then
 *  the runtime broker delivers to the dynamic subscriber. Receivers and publisher are written as for the broker. */
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
struct Probe final : sub0::Subscribe<Sample> {
    void receive(const Sample& s) noexcept { COLLAPSE_WORK(s.value + 11U); }
};
ControllerSlot controller;
LoggerSlot logger;
collapse::Slot<Probe> probe;
collapse::Slot<Sensor> sensor;
}

COLLAPSE_ENTRY void collapse_setup() { controller.emplace(); logger.emplace(); probe.emplace(); sensor.emplace(); }
COLLAPSE_ENTRY void collapse_publish(uint32_t v) { sensor->send(collapse::arg(v)); }
COLLAPSE_ENTRY void collapse_teardown() { sensor.reset(); probe.reset(); logger.reset(); controller.reset(); }
