#pragma once
/** Case cross_file, candidate "typed route": the body of every variant's app.cpp: the receiver objects, the
 *  publisher and the entry points. */
#include "route_topology.hpp"

namespace app {
ControllerSlot controllerA;
ControllerSlot controllerB;
LoggerSlot logger;
}

namespace {
struct Sensor : sub0::spike::Publish<app::Sample> {
    void send(uint32_t v) noexcept { sub0::spike::publish(*this, app::Sample{v}); }
};
collapse::Slot<Sensor> sensor;
}

COLLAPSE_ENTRY void collapse_setup() { sensor.emplace(); app::controllerA.emplace(3U); app::controllerB.emplace(5U); app::logger.emplace(); }
COLLAPSE_ENTRY void collapse_publish(uint32_t v) { sensor->send(collapse::arg(v)); }
COLLAPSE_ENTRY void collapse_teardown() { app::logger.reset(); app::controllerB.reset(); app::controllerA.reset(); sensor.reset(); }
