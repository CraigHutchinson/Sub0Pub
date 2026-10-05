/** Case: default and runtime filters. A controller accepts everything (default filter: must cost nothing);
 *  an even-only monitor keeps its necessary branch. Reference: the equivalent direct code. */
#include "collapse_case.hpp"

namespace {
struct Sample { uint32_t value; };
struct Controller {
    void receive(const Sample& s) noexcept { COLLAPSE_WORK(s.value * 3U); }
};
struct EvenMonitor {
    void receive(const Sample& s) noexcept { COLLAPSE_WORK(s.value + 7U); }
};
collapse::Slot<Controller> controller;
collapse::Slot<EvenMonitor> monitor;
}

COLLAPSE_ENTRY void collapse_setup() { controller.emplace(); monitor.emplace(); }
COLLAPSE_ENTRY void collapse_publish(uint32_t v)
{
    const Sample s{collapse::arg(v)};
    controller->receive(s);
    if ((s.value & 1U) == 0U)
        monitor->receive(s);
}
COLLAPSE_ENTRY void collapse_teardown() { monitor.reset(); controller.reset(); }
