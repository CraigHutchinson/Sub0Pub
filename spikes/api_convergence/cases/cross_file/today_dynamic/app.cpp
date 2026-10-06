#include "receivers.hpp"

namespace {
struct Sensor : sub0::Publish<app::Sample> {
    void send(uint32_t v) noexcept { sub0::publish(*this, app::Sample{v}); }
};
collapse::Slot<Sensor> sensor;
collapse::Slot<app::Controller> controllerA;
collapse::Slot<app::Controller> controllerB;
collapse::Slot<app::Logger> logger;
}

COLLAPSE_ENTRY void collapse_setup() { sensor.emplace(); controllerA.emplace(3U); controllerB.emplace(5U); logger.emplace(); }
COLLAPSE_ENTRY void collapse_publish(uint32_t v) { sensor->send(collapse::arg(v)); }
COLLAPSE_ENTRY void collapse_teardown() { logger.reset(); controllerB.reset(); controllerA.reset(); sensor.reset(); }
