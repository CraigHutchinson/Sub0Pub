// COLLAPSE_REFERENCE: handwritten_erased
/** Case: two independent domains. Pattern B3: one non-template publisher type, one instance per domain, each
 *  holding a Sink<Sample> into its domain's typed wiring. */
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
struct Sensor {
    explicit Sensor(sub0::Sink<Sample> o) noexcept : out(o) {}
    void send(uint32_t v) noexcept { out.publish(Sample{v}); }
    sub0::Sink<Sample> out;
};
using BusA = sub0::Wiring<Controller, Logger>;
using BusB = sub0::Wiring<Controller>;
collapse::Slot<Controller> controllerA;
collapse::Slot<Logger> loggerA;
collapse::Slot<Controller> controllerB;
collapse::Slot<BusA> busA;
collapse::Slot<BusB> busB;
collapse::Slot<Sensor> sensorA;
collapse::Slot<Sensor> sensorB;
}

COLLAPSE_ENTRY void collapse_setup()
{
    controllerA.emplace(3U); loggerA.emplace(); controllerB.emplace(5U);
    busA.emplace(controllerA.get(), loggerA.get());
    busB.emplace(controllerB.get());
    sensorA.emplace(sub0::Sink<Sample>(busA.get()));
    sensorB.emplace(sub0::Sink<Sample>(busB.get()));
}
COLLAPSE_ENTRY void collapse_publish(uint32_t v)
{
    sensorA->send(collapse::arg(v));
    sensorB->send(collapse::arg(v + 1U));
}
COLLAPSE_ENTRY void collapse_teardown()
{
    sensorB.reset(); sensorA.reset(); busB.reset(); busA.reset();
    controllerB.reset(); loggerA.reset(); controllerA.reset();
}
