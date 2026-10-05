/** Case: one receiver. Runtime-bound reference: the same work, with receiver addresses stored at setup and called through,
 *  as a careful engineer writes it when addresses are only known at run time. Equal work for patterns whose
 *  bindings are runtime values (B1 `wire(...)`), which select it with `// COLLAPSE_REFERENCE: handwritten_runtime`. */
#include "collapse_case.hpp"

namespace {
struct Sample { uint32_t value; };
struct Controller {
    explicit Controller(uint32_t g) noexcept : gain(g) {}
    void receive(const Sample& s) noexcept { COLLAPSE_WORK(s.value * gain); }
    uint32_t gain;
};
collapse::Slot<Controller> controller;
struct Sensor {
    Controller* c;
    void send(uint32_t v) noexcept { c->receive(Sample{v}); }
};
collapse::Slot<Sensor> sensor;
}

COLLAPSE_ENTRY void collapse_setup() { controller.emplace(3U); sensor.emplace(Sensor{&controller.get()}); }
COLLAPSE_ENTRY void collapse_publish(uint32_t v) { sensor->send(collapse::arg(v)); }
COLLAPSE_ENTRY void collapse_teardown() { sensor.reset(); controller.reset(); }
