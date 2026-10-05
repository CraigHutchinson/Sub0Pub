/** Case: 32 receivers of one type. Extra reference (not selected by any variant): the receivers in one static
 *  array, delivered by a loop -- what a careful engineer writes for a homogeneous fan-out. Reported against
 *  `handwritten` (one call per receiver), it prices unrolled delivery, which is the only form static wiring has. */
#include "collapse_case.hpp"
#include "many.hpp"

namespace {
struct Sample { uint32_t value; };
struct Controller {
    explicit Controller(uint32_t g) noexcept : gain(g) {}
    void receive(const Sample& s) noexcept { COLLAPSE_WORK(s.value * gain); }
    uint32_t gain;
};
collapse::Slot<Controller> controllers[MANY_COUNT];
}

COLLAPSE_ENTRY void collapse_setup() { for (uint32_t i = 0; i < MANY_COUNT; ++i) controllers[i].emplace(i + 1U); }
COLLAPSE_ENTRY void collapse_publish(uint32_t v)
{
    const Sample s{collapse::arg(v)};
    for (auto& c : controllers)
        c->receive(s);
}
COLLAPSE_ENTRY void collapse_teardown() { for (auto& c : controllers) c.reset(); }
