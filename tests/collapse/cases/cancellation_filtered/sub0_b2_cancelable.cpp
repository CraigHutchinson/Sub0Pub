/** Case: cancellation combined with filters. Gate only sees odd values (filter) and stops the rest of the
 *  publication when the value is also a multiple of 3; Controller (gain 3) only sees even values (filter);
 *  Logger sees everything that was not stopped. Bound order gate, controller, logger.
 *  Pattern B2 with publishCancelable: the filters and the stop compose (a filtered-out gate cannot stop the publication). */
#include "collapse_case.hpp"
#include "sub0pub/sub0pub.hpp"

namespace {
struct Sample { uint32_t value; };
struct Gate {
    bool filter(const Sample& s) const noexcept { return (s.value & 1U) != 0U; }
    bool receive(const Sample& s) noexcept
    {
        COLLAPSE_WORK(s.value + 1U);
        return (s.value % 3U) != 0U; // false stops the rest
    }
};
struct Controller {
    explicit Controller(uint32_t g) noexcept : gain(g) {}
    bool filter(const Sample& s) const noexcept { return (s.value & 1U) == 0U; }
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
using Bus = sub0::StaticWiring<&gate, &controller, &logger>;
}

COLLAPSE_ENTRY void collapse_setup() { gate.emplace(); controller.emplace(3U); logger.emplace(); }
COLLAPSE_ENTRY void collapse_publish(uint32_t v) { Bus::publishCancelable(Sample{collapse::arg(v)}); }
COLLAPSE_ENTRY void collapse_teardown() { logger.reset(); controller.reset(); gate.reset(); }
