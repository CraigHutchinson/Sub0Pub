// COLLAPSE_REFERENCE: handwritten_erased
#include "receivers.hpp"

namespace {
using Bus = sub0::Wiring<app::Controller, app::Controller, app::Logger>;
collapse::Slot<app::Controller> controllerA;
collapse::Slot<app::Controller> controllerB;
collapse::Slot<app::Logger> logger;
collapse::Slot<Bus> bus;
collapse::Slot<app::Sensor> sensor;
}

COLLAPSE_ENTRY void collapse_setup()
{
    controllerA.emplace(3U); controllerB.emplace(5U); logger.emplace();
    bus.emplace(controllerA.get(), controllerB.get(), logger.get());
    sensor.emplace(sub0::Sink<app::Sample>(bus.get()));
}
COLLAPSE_ENTRY void collapse_publish(uint32_t v) { sensor->send(collapse::arg(v)); }
COLLAPSE_ENTRY void collapse_teardown() { sensor.reset(); bus.reset(); logger.reset(); controllerB.reset(); controllerA.reset(); }
