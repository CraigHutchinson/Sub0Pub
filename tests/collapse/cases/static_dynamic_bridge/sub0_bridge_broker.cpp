/** Static/dynamic bridge through a BrokerPort bound into the static wiring: it forwards to the runtime broker
 *  (Domain/Subscribe/Publish) for the dynamic subscriber. */
#include "collapse_case.hpp"
#include "sub0pub/sub0pub.hpp"

namespace {
struct Sample { uint32_t value; using sub0_config = sub0::config<sub0::Scoped, sub0::Direct, sub0::NoContext, sub0::NoFilter>; }; // lean registry: only the features the hand-written registry has
struct Controller {
    void receive(const Sample& s) noexcept { COLLAPSE_WORK(s.value * 3U); }
};
struct Logger {
    void receive(const Sample& s) noexcept { ++count; COLLAPSE_WORK(s.value ^ count); }
    uint32_t count = 0;
};
struct Probe final : sub0::Subscribe<Sample> {
    explicit Probe(sub0::Domain<Sample>& d) noexcept : sub0::Subscribe<Sample>(d) {}
    void receive(const Sample& s) noexcept override { COLLAPSE_WORK(s.value + 11U); }
};

collapse::Slot<Controller> controller;
collapse::Slot<Logger> logger;
collapse::Slot<sub0::Domain<Sample>> domain;
collapse::Slot<sub0::BrokerPort<Sample>> port;
collapse::Slot<Probe> probe;
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
    probe.emplace(domain.get());
    sensor.emplace();
}
COLLAPSE_ENTRY void collapse_publish(uint32_t v) { sensor->send(collapse::arg(v)); }
COLLAPSE_ENTRY void collapse_teardown()
{
    sensor.reset();
    probe.reset();
    port.reset();
    domain.reset();
    logger.reset();
    controller.reset();
}
