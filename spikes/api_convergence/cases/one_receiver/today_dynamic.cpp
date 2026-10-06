/** Case: one concrete receiver. The runtime broker (Subscribe/Publish). */
#include "collapse_case.hpp"
#include "sub0pub/sub0pub.hpp"

namespace {
struct Sample { uint32_t value; };
struct Controller final : sub0::Subscribe<Sample> {
    explicit Controller(uint32_t g) noexcept : gain(g) {}
    void receive(const Sample& s) noexcept override { COLLAPSE_WORK(s.value * gain); }
    uint32_t gain;
};
struct Sensor : sub0::Publish<Sample> {
    void send(uint32_t v) noexcept { sub0::publish(*this, Sample{v}); }
};
collapse::Slot<Sensor> sensor;
collapse::Slot<Controller> controller;
}

COLLAPSE_ENTRY void collapse_setup() { sensor.emplace(); controller.emplace(3U); }
COLLAPSE_ENTRY void collapse_publish(uint32_t v) { sensor->send(collapse::arg(v)); }
COLLAPSE_ENTRY void collapse_teardown() { controller.reset(); sensor.reset(); }
