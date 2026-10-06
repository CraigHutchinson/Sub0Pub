#include "receivers.hpp"

namespace {
collapse::Slot<app::Controller> controllerA;
collapse::Slot<app::Controller> controllerB;
collapse::Slot<app::Logger> logger;
using Bus = sub0::StaticWiring<&controllerA, &controllerB, &logger>;
template<class Out>
struct Sensor {
    void send(uint32_t v) noexcept { Out::publish(app::Sample{v}); }
};
collapse::Slot<Sensor<Bus>> sensor;
}

COLLAPSE_ENTRY void collapse_setup() { controllerA.emplace(3U); controllerB.emplace(5U); logger.emplace(); sensor.emplace(); }
COLLAPSE_ENTRY void collapse_publish(uint32_t v) { sensor->send(collapse::arg(v)); }
COLLAPSE_ENTRY void collapse_teardown() { sensor.reset(); logger.reset(); controllerB.reset(); controllerA.reset(); }
