// COLLAPSE_REFERENCE: handwritten_erased
/** Case: default and runtime filters: Controller declares an always-true filter (must compile away); EvenMonitor's runtime filter keeps its branch.
 *  Pattern B3: non-template publisher holding a type-erased Sink<Sample> into typed wiring (public API, include/sub0pub/sub0pub.hpp). */
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

struct Sensor {
    explicit Sensor(sub0::Sink<Sample> o) noexcept : out(o) {}
    void send(uint32_t v) noexcept { out.publish(Sample{v}); }
    sub0::Sink<Sample> out;
};
using Bus = sub0::Wiring<Controller, EvenMonitor>;
collapse::Slot<Controller> controller;
collapse::Slot<EvenMonitor> monitor;
collapse::Slot<Bus> bus;
collapse::Slot<Sensor> sensor;
}

COLLAPSE_ENTRY void collapse_setup() { controller.emplace(); monitor.emplace(); bus.emplace(controller.get(), monitor.get()); sensor.emplace(sub0::Sink<Sample>(bus.get())); }
COLLAPSE_ENTRY void collapse_publish(uint32_t v) { sensor->send(collapse::arg(v)); }
COLLAPSE_ENTRY void collapse_teardown() { sensor.reset(); bus.reset(); monitor.reset(); controller.reset(); }
