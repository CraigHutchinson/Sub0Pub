/** Case: dynamic subscriptions. The runtime broker (Subscribe/Publish) (runtime registry). */
// Today's API at its leanest settings (fair comparison): direct dispatch, no assertion checks
#define SUB0PUB_REENTRANT_SAFE false
#define SUB0PUB_ASSERT false
#include "collapse_case.hpp"
#include "sub0pub/sub0pub.hpp"

namespace {
struct Sample { uint32_t value; };
struct Controller final : sub0::Subscribe<Sample> {
    void receive(const Sample& s) noexcept override { COLLAPSE_WORK(s.value * 3U); }
};
struct Probe final : sub0::Subscribe<Sample> {
    void receive(const Sample& s) noexcept override { COLLAPSE_WORK(s.value + 11U); }
};
struct Sensor : sub0::Publish<Sample> {
    void send(const Sample& s) noexcept { sub0::publish(*this, s); }
};
collapse::Slot<Sensor> sensor;
collapse::Slot<Controller> controller;
}

COLLAPSE_ENTRY void collapse_setup() { sensor.emplace(); controller.emplace(); }
COLLAPSE_ENTRY void collapse_publish(uint32_t v)
{
    const Sample s{collapse::arg(v)};
    if ((s.value & 3U) == 0U)
    {
        Probe probe;
        sensor->send(s);
    }
    else
        sensor->send(s);
}
COLLAPSE_ENTRY void collapse_teardown() { controller.reset(); sensor.reset(); }
