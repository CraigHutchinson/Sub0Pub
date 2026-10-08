/** Case: static/dynamic bridge with several dynamic subscribers and churn. Two static receivers (controller,
 *  logger); two dynamic probes (offsets 11 and 13) subscribed at setup; a third, transient probe (offset 17)
 *  exists only for publications where (v & 3) == 0 (subscribed before, unsubscribed after). Order:
 *  controller, logger, then dynamic subscribers in subscription order.
 *  The bridge is on the message type: sub0::StaticFirst calls the two fixed receivers directly, then the runtime
 *  broker (lean configuration); dynamic subscribers join and leave by construction and destruction. */
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
    explicit Probe(uint32_t o) noexcept : offset(o) {}
    void receive(const Sample& s) noexcept { COLLAPSE_WORK(s.value + offset); }
    uint32_t offset;
};
ControllerSlot controller;
LoggerSlot logger;
collapse::Slot<Probe> probeA;
collapse::Slot<Probe> probeB;
collapse::Slot<Sensor> sensor;
}

COLLAPSE_ENTRY void collapse_setup()
{
    controller.emplace(); logger.emplace(); probeA.emplace(11U); probeB.emplace(13U); sensor.emplace();
}
COLLAPSE_ENTRY void collapse_publish(uint32_t v)
{
    const uint32_t value = collapse::arg(v);
    if ((value & 3U) == 0U)
    {
        Probe transient(17U);
        sensor->send(value);
    }
    else
        sensor->send(value);
}
COLLAPSE_ENTRY void collapse_teardown()
{
    sensor.reset(); probeB.reset(); probeA.reset(); logger.reset(); controller.reset();
}
