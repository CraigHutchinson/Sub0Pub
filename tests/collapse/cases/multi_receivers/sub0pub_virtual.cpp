/** Case: multiple receivers including a repeated type. The runtime broker (Subscribe/Publish). */
#include "collapse_case.hpp"
#include "sub0pub/sub0pub.hpp"

namespace {
struct Sample { uint32_t value; };
struct Controller final : sub0::Subscribe<Sample> {
    explicit Controller(uint32_t g) noexcept : gain(g) {}
    void receive(const Sample& s) noexcept override { COLLAPSE_WORK(s.value * gain); }
    uint32_t gain;
};
struct Logger final : sub0::Subscribe<Sample> {
    void receive(const Sample& s) noexcept override { ++count; COLLAPSE_WORK(s.value ^ count); }
    uint32_t count = 0;
};
struct Sensor : sub0::Publish<Sample> {
    void send(uint32_t v) noexcept { sub0::publish(*this, Sample{v}); }
};
collapse::Slot<Sensor> sensor;
collapse::Slot<Controller> controllerA;
collapse::Slot<Controller> controllerB;
collapse::Slot<Logger> logger;
}

COLLAPSE_ENTRY void collapse_setup()
{
    sensor.emplace();
    controllerA.emplace(3U); // subscription order = dispatch order
    controllerB.emplace(5U);
    logger.emplace();
}
COLLAPSE_ENTRY void collapse_publish(uint32_t v) { sensor->send(collapse::arg(v)); }
COLLAPSE_ENTRY void collapse_teardown() { logger.reset(); controllerB.reset(); controllerA.reset(); sensor.reset(); }
