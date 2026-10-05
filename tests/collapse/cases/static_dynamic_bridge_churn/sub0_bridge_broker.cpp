/** Case: static/dynamic bridge with several dynamic subscribers and churn. Two static receivers (controller,
 *  logger); two dynamic probes (offsets 11 and 13) subscribed at setup; a third, transient probe (offset 17)
 *  exists only for publications where (v & 3) == 0 (subscribed before, unsubscribed after). Order:
 *  controller, logger, then dynamic subscribers in subscription order.
 *  A BrokerPort bound into the static wiring forwards to the runtime broker (lean configuration: the
 *  hand-written registry's features); dynamic subscribers join and leave by construction and destruction. */
#include "collapse_case.hpp"
#include "sub0pub/sub0pub.hpp"

namespace {
struct Sample { uint32_t value; using sub0_config = sub0::config<sub0::Scoped, sub0::Direct, sub0::NoContext, sub0::NoFilter>; };
struct Controller {
    void receive(const Sample& s) noexcept { COLLAPSE_WORK(s.value * 3U); }
};
struct Logger {
    void receive(const Sample& s) noexcept { ++count; COLLAPSE_WORK(s.value ^ count); }
    uint32_t count = 0;
};
struct Probe final : sub0::Subscribe<Sample> {
    Probe(sub0::Domain<Sample>& d, uint32_t o) noexcept : sub0::Subscribe<Sample>(d), offset(o) {}
    void receive(const Sample& s) noexcept override { COLLAPSE_WORK(s.value + offset); }
    uint32_t offset;
};
collapse::Slot<Controller> controller;
collapse::Slot<Logger> logger;
collapse::Slot<sub0::Domain<Sample>> domain;
collapse::Slot<sub0::BrokerPort<Sample>> port;
collapse::Slot<Probe> probeA;
collapse::Slot<Probe> probeB;
using Bus = sub0::StaticWiring<&controller, &logger, &port>;
}

COLLAPSE_ENTRY void collapse_setup()
{
    controller.emplace(); logger.emplace(); domain.emplace(); port.emplace(domain.get());
    probeA.emplace(domain.get(), 11U); probeB.emplace(domain.get(), 13U);
}
COLLAPSE_ENTRY void collapse_publish(uint32_t v)
{
    const Sample s{collapse::arg(v)};
    if ((s.value & 3U) == 0U)
    {
        Probe transient(domain.get(), 17U);
        Bus::publish(s);
    }
    else
        Bus::publish(s);
}
COLLAPSE_ENTRY void collapse_teardown()
{
    probeB.reset(); probeA.reset(); port.reset(); domain.reset(); logger.reset(); controller.reset();
}
