/** Case: static/dynamic bridge with several dynamic subscribers and churn. Two static receivers (controller,
 *  logger); two dynamic probes (offsets 11 and 13) subscribed at setup; a third, transient probe (offset 17)
 *  exists only for publications where (v & 3) == 0 (subscribed before, unsubscribed after). Order:
 *  controller, logger, then dynamic subscribers in subscription order.
 *  A DynamicPort bound into the static wiring (public API, include/sub0pub/sub0pub.hpp). */
#include "collapse_case.hpp"
#include "sub0pub/sub0pub.hpp"

namespace {
struct Sample { uint32_t value; };
struct Controller {
    void receive(const Sample& s) noexcept { COLLAPSE_WORK(s.value * 3U); }
};
struct Logger {
    void receive(const Sample& s) noexcept { ++count; COLLAPSE_WORK(s.value ^ count); }
    uint32_t count = 0;
};
struct Probe final : sub0::DynamicPort<Sample>::Receiver {
    explicit Probe(uint32_t o) noexcept : offset(o) {}
    void receive(const Sample& s) noexcept override { COLLAPSE_WORK(s.value + offset); }
    uint32_t offset;
};
collapse::Slot<Controller> controller;
collapse::Slot<Logger> logger;
collapse::Slot<sub0::DynamicPort<Sample>> port;
collapse::Slot<Probe> probeA;
collapse::Slot<Probe> probeB;
using Bus = sub0::StaticWiring<&controller, &logger, &port>;
}

COLLAPSE_ENTRY void collapse_setup()
{
    controller.emplace(); logger.emplace(); port.emplace();
    probeA.emplace(11U); probeB.emplace(13U);
    port->add(&probeA.get()); port->add(&probeB.get());
}
COLLAPSE_ENTRY void collapse_publish(uint32_t v)
{
    const Sample s{collapse::arg(v)};
    if ((s.value & 3U) == 0U)
    {
        Probe transient(17U);
        port->add(&transient);
        Bus::publish(s);
        port->remove(&transient);
    }
    else
        Bus::publish(s);
}
COLLAPSE_ENTRY void collapse_teardown()
{
    port->remove(&probeB.get()); port->remove(&probeA.get());
    probeB.reset(); probeA.reset(); port.reset(); logger.reset(); controller.reset();
}
