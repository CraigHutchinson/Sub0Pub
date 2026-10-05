// COLLAPSE_REFERENCE: handwritten_runtime
/** Case: two independent domains. Pattern B1: one typed wiring per domain, bound at the composition point. */
#include "collapse_case.hpp"
#include "sub0pub/sub0pub.hpp"

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
template<class Out>
struct Sensor {
    explicit Sensor(const Out& o) noexcept : out(o) {}
    void send(uint32_t v) noexcept { out.publish(Sample{v}); }
    Out out; // the wiring held by value: a tuple of receiver references, one hop to each receiver
};
using BusA = sub0::Wiring<Controller, Logger>;
using BusB = sub0::Wiring<Controller>;
collapse::Slot<Controller> controllerA;
collapse::Slot<Logger> loggerA;
collapse::Slot<Controller> controllerB;
collapse::Slot<Sensor<BusA>> sensorA;
collapse::Slot<Sensor<BusB>> sensorB;
}

COLLAPSE_ENTRY void collapse_setup()
{
    controllerA.emplace(3U); loggerA.emplace(); controllerB.emplace(5U);
    sensorA.emplace(BusA(controllerA.get(), loggerA.get()));
    sensorB.emplace(BusB(controllerB.get()));
}
COLLAPSE_ENTRY void collapse_publish(uint32_t v)
{
    sensorA->send(collapse::arg(v));
    sensorB->send(collapse::arg(v + 1U));
}
COLLAPSE_ENTRY void collapse_teardown()
{
    sensorB.reset(); sensorA.reset();
    controllerB.reset(); loggerA.reset(); controllerA.reset();
}
