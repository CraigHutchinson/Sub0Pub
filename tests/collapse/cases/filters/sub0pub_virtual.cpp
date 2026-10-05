/** Case: default and runtime filters. The runtime broker (Subscribe/Publish) (filter() is virtual). */
#define SUB0PUB_FILTER true // filter() is opt-in; this case measures it (types are local to this TU)
#include "collapse_case.hpp"
#include "sub0pub/sub0pub.hpp"

namespace {
struct Sample { uint32_t value; };
struct Controller final : sub0::Subscribe<Sample> {
    void receive(const Sample& s) noexcept override { COLLAPSE_WORK(s.value * 3U); }
};
struct EvenMonitor final : sub0::Subscribe<Sample> {
    bool filter(const Sample& s) noexcept override { return (s.value & 1U) == 0U; }
    void receive(const Sample& s) noexcept override { COLLAPSE_WORK(s.value + 7U); }
};
struct Sensor : sub0::Publish<Sample> {
    void send(uint32_t v) noexcept { sub0::publish(*this, Sample{v}); }
};
collapse::Slot<Sensor> sensor;
collapse::Slot<Controller> controller;
collapse::Slot<EvenMonitor> monitor;
}

COLLAPSE_ENTRY void collapse_setup() { sensor.emplace(); controller.emplace(); monitor.emplace(); }
COLLAPSE_ENTRY void collapse_publish(uint32_t v) { sensor->send(collapse::arg(v)); }
COLLAPSE_ENTRY void collapse_teardown() { monitor.reset(); controller.reset(); sensor.reset(); }
