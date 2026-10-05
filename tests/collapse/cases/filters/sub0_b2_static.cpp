/** Case: default and runtime filters: Controller declares an always-true filter (must compile away); EvenMonitor's runtime filter keeps its branch.
 *  Pattern B2: static topology: static-storage receivers bound as template arguments (public API, include/sub0pub/sub0pub.hpp). */
#include "collapse_case.hpp"
#include "sub0pub/sub0pub.hpp"

namespace {
struct Sample { uint32_t value; };
struct Controller {
    bool filter(const Sample&) const noexcept { return true; }
    void receive(const Sample& s) noexcept { COLLAPSE_WORK(s.value * 3U); }
};
struct EvenMonitor {
    bool filter(const Sample& s) const noexcept { return (s.value & 1U) == 0U; }
    void receive(const Sample& s) noexcept { COLLAPSE_WORK(s.value + 7U); }
};

collapse::Slot<Controller> controller;
collapse::Slot<EvenMonitor> monitor;
using Bus = sub0::StaticWiring<&controller, &monitor>;
template<class Out>
struct Sensor {
    void send(uint32_t v) noexcept { Out::publish(Sample{v}); }
};
collapse::Slot<Sensor<Bus>> sensor;
}

COLLAPSE_ENTRY void collapse_setup() { controller.emplace(); monitor.emplace(); sensor.emplace(); }
COLLAPSE_ENTRY void collapse_publish(uint32_t v) { sensor->send(collapse::arg(v)); }
COLLAPSE_ENTRY void collapse_teardown() { sensor.reset(); monitor.reset(); controller.reset(); }
