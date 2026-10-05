/** Case: two domains (independent sessions of the same message type). Runtime-bound reference: the same work, with receiver addresses stored at setup and called through,
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
struct Logger {
    void receive(const Sample& s) noexcept { ++count; COLLAPSE_WORK(s.value ^ count); }
    uint32_t count = 0;
};
collapse::Slot<Controller> controllerA;
collapse::Slot<Logger> loggerA;
collapse::Slot<Controller> controllerB;
struct SensorA {
    Controller* c;
    Logger* log;
    void send(uint32_t v) noexcept { const Sample s{v}; c->receive(s); log->receive(s); }
};
struct SensorB {
    Controller* c;
    void send(uint32_t v) noexcept { c->receive(Sample{v}); }
};
collapse::Slot<SensorA> sensorA;
collapse::Slot<SensorB> sensorB;
}

COLLAPSE_ENTRY void collapse_setup()
{
    controllerA.emplace(3U); loggerA.emplace(); controllerB.emplace(5U);
    sensorA.emplace(SensorA{&controllerA.get(), &loggerA.get()});
    sensorB.emplace(SensorB{&controllerB.get()});
}
COLLAPSE_ENTRY void collapse_publish(uint32_t v)
{
    sensorA->send(collapse::arg(v));
    sensorB->send(collapse::arg(v + 1U));
}
COLLAPSE_ENTRY void collapse_teardown() { sensorB.reset(); sensorA.reset(); controllerB.reset(); loggerA.reset(); controllerA.reset(); }
