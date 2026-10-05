// COLLAPSE_REFERENCE: handwritten_runtime
#include "receivers.hpp"

namespace {
template<class Out>
struct Sensor {
    explicit Sensor(const Out& o) noexcept : out(o) {}
    void send(uint32_t v) noexcept { out.publish(app::Sample{v}); }
    Out out; // the wiring held by value: one hop to each receiver
};
using Bus = sub0::Wiring<app::Controller, app::Controller, app::Logger>;
collapse::Slot<app::Controller> controllerA;
collapse::Slot<app::Controller> controllerB;
collapse::Slot<app::Logger> logger;
collapse::Slot<Sensor<Bus>> sensor;
}

COLLAPSE_ENTRY void collapse_setup() { controllerA.emplace(3U); controllerB.emplace(5U); logger.emplace(); sensor.emplace(Bus(controllerA.get(), controllerB.get(), logger.get())); }
COLLAPSE_ENTRY void collapse_publish(uint32_t v) { sensor->send(collapse::arg(v)); }
COLLAPSE_ENTRY void collapse_teardown() { sensor.reset(); logger.reset(); controllerB.reset(); controllerA.reset(); }
