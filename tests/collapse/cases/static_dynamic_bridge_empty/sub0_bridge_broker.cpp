// COLLAPSE_REFERENCE: handwritten_registry
/** Static/dynamic bridge through a BrokerPort, empty dynamic side: the BrokerPort and its Domain are still bound
 *  and constructed, but no dynamic subscriber is ever added. Checks whether an unused runtime broker collapses. */
#include "collapse_case.hpp"
#include "sub0pub/sub0pub.hpp"

namespace {
struct Sample { uint32_t value; using sub0_config = sub0::config<sub0::Scoped, sub0::Direct, sub0::NoContext, sub0::NoFilter, sub0::AllowNoReceivers>; }; // lean registry: only the features the hand-written registry has; its runtime side stays empty, by design
struct Controller {
    void receive(const Sample& s) noexcept { COLLAPSE_WORK(s.value * 3U); }
};
struct Logger {
    void receive(const Sample& s) noexcept { ++count; COLLAPSE_WORK(s.value ^ count); }
    uint32_t count = 0;
};

collapse::Slot<Controller> controller;
collapse::Slot<Logger> logger;
collapse::Slot<sub0::Domain<Sample>> domain;
collapse::Slot<sub0::BrokerPort<Sample>> port;
using Bus = sub0::StaticWiring<&controller, &logger, &port>;
template<class Out>
struct Sensor {
    void send(uint32_t v) noexcept { Out::publish(Sample{v}); }
};
collapse::Slot<Sensor<Bus>> sensor;
}

COLLAPSE_ENTRY void collapse_setup()
{
    controller.emplace();
    logger.emplace();
    domain.emplace();
    port.emplace(domain.get());
    sensor.emplace();
}
COLLAPSE_ENTRY void collapse_publish(uint32_t v) { sensor->send(collapse::arg(v)); }
COLLAPSE_ENTRY void collapse_teardown()
{
    sensor.reset();
    port.reset();
    domain.reset();
    logger.reset();
    controller.reset();
}
