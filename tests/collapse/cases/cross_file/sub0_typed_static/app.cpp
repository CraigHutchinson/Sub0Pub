#include "receivers.hpp"

namespace app {
ControllerSlot controllerA;
ControllerSlot controllerB;
LoggerSlot logger;
}

namespace {
struct Sensor : sub0::Publish<app::Sample> {
    void send(uint32_t v) noexcept { sub0::publish(*this, app::Sample{v}); }
};
collapse::Slot<Sensor> sensor;
}

COLLAPSE_ENTRY void collapse_setup() { app::controllerA.emplace(3U); app::controllerB.emplace(5U); app::logger.emplace(); sensor.emplace(); }
COLLAPSE_ENTRY void collapse_publish(uint32_t v) { sensor->send(collapse::arg(v)); }
COLLAPSE_ENTRY void collapse_teardown() { sensor.reset(); app::logger.reset(); app::controllerB.reset(); app::controllerA.reset(); }
