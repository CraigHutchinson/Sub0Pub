/** Case: two independent domains (sessions) carrying the same message type. Domain A: controller (gain 3) and
 *  logger; domain B: controller (gain 5). Each publication goes to one domain only (no cross-talk).
 *  Reference: direct calls per domain. Today's sub0pub.hpp API has one global table per type: not expressible. */
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
}

COLLAPSE_ENTRY void collapse_setup() { controllerA.emplace(3U); loggerA.emplace(); controllerB.emplace(5U); }
COLLAPSE_ENTRY void collapse_publish(uint32_t v)
{
    const Sample a{collapse::arg(v)};
    controllerA->receive(a);
    loggerA->receive(a);
    const Sample b{collapse::arg(v + 1U)};
    controllerB->receive(b);
}
COLLAPSE_ENTRY void collapse_teardown() { controllerB.reset(); loggerA.reset(); controllerA.reset(); }
