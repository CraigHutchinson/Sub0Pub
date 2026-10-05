/** Case: filters (controller always, monitor only for even values). Runtime-bound reference: the same work, with receiver addresses stored at setup and called through,
 *  as a careful engineer writes it when addresses are only known at run time. Equal work for patterns whose
 *  bindings are runtime values (B1 `wire(...)`), which select it with `// COLLAPSE_REFERENCE: handwritten_runtime`. */
#include "collapse_case.hpp"

namespace {
struct Sample { uint32_t value; };
struct Controller {
    void receive(const Sample& s) noexcept { COLLAPSE_WORK(s.value * 3U); }
};
struct EvenMonitor {
    void receive(const Sample& s) noexcept { COLLAPSE_WORK(s.value + 7U); }
};
collapse::Slot<Controller> controller;
collapse::Slot<EvenMonitor> monitor;
struct Sensor {
    Controller* c;
    EvenMonitor* m;
    void send(uint32_t v) noexcept
    {
        const Sample s{v};
        c->receive(s);
        if ((s.value & 1U) == 0U)
            m->receive(s);
    }
};
collapse::Slot<Sensor> sensor;
}

COLLAPSE_ENTRY void collapse_setup() { controller.emplace(); monitor.emplace(); sensor.emplace(Sensor{&controller.get(), &monitor.get()}); }
COLLAPSE_ENTRY void collapse_publish(uint32_t v) { sensor->send(collapse::arg(v)); }
COLLAPSE_ENTRY void collapse_teardown() { sensor.reset(); monitor.reset(); controller.reset(); }
