/** Case: receiver-controlled early stop of the current publication.
 *  Receivers in bound order: Gate (does its own work first, then cancels the rest of the publication when
 *  value % 3 == 0), Controller (gain 3), Logger. Reference: direct calls with an early return after Gate's
 *  decision — Controller and Logger are simply not called for cancelled publications. The decision is returned
 *  by value, as a careful engineer would write it (storing it in a Gate member would cost 3 instructions and 8 B
 *  of RAM, and set the bar too low). */
#include "collapse_case.hpp"

namespace {
struct Sample { uint32_t value; };
struct Gate {
    bool receive(const Sample& s) noexcept // true: stop this publication here
    {
        COLLAPSE_WORK(s.value);
        return (s.value % 3U) == 0U;
    }
};
struct Controller {
    explicit Controller(uint32_t g) noexcept : gain(g) {}
    void receive(const Sample& s) noexcept { COLLAPSE_WORK(s.value * gain); }
    uint32_t gain;
};
struct Logger {
    void receive(const Sample& s) noexcept { ++count; COLLAPSE_WORK(s.value ^ count); }
    uint32_t count = 0;
};
collapse::Slot<Gate> gate;
collapse::Slot<Controller> controller;
collapse::Slot<Logger> logger;
}

COLLAPSE_ENTRY void collapse_setup() { gate.emplace(); controller.emplace(3U); logger.emplace(); }
COLLAPSE_ENTRY void collapse_publish(uint32_t v)
{
    const Sample s{collapse::arg(v)};
    if (gate->receive(s))
        return;
    controller->receive(s);
    logger->receive(s);
}
COLLAPSE_ENTRY void collapse_teardown() { logger.reset(); controller.reset(); gate.reset(); }
