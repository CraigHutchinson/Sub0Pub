/** Case: cancellation combined with filters. Gate only sees odd values (filter) and stops the rest of the
 *  publication when the value is also a multiple of 3; Controller (gain 3) only sees even values (filter);
 *  Logger sees everything that was not stopped. Bound order gate, controller, logger.
 *  Reference: direct calls with the filters and the early return written out. */
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
struct Gate {
    bool receive(const Sample& s) noexcept // true: stop
    {
        COLLAPSE_WORK(s.value + 1U);
        return (s.value % 3U) == 0U;
    }
};
collapse::Slot<Gate> gate;
collapse::Slot<Controller> controller;
collapse::Slot<Logger> logger;
}

COLLAPSE_ENTRY void collapse_setup() { gate.emplace(); controller.emplace(3U); logger.emplace(); }
COLLAPSE_ENTRY void collapse_publish(uint32_t v)
{
    const Sample s{collapse::arg(v)};
    if ((s.value & 1U) != 0U && gate->receive(s))
        return;
    if ((s.value & 1U) == 0U)
        controller->receive(s);
    logger->receive(s);
}
COLLAPSE_ENTRY void collapse_teardown() { logger.reset(); controller.reset(); gate.reset(); }
