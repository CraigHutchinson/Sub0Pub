/** Case: static/dynamic bridge, empty dynamic side. Same scenario as static_dynamic_bridge, but no dynamic
 *  subscriber is ever present. Reference: the ideal equal-work code for this case has no registry at all --
 *  just the two direct calls. Variants keep their bridge element bound but never add a dynamic subscriber,
 *  to check the static path pays ~nothing for a dynamic side that stays empty. */
#include "collapse_case.hpp"

namespace {
struct Sample { uint32_t value; };
struct Controller {
    void receive(const Sample& s) noexcept { COLLAPSE_WORK(s.value * 3U); }
};
struct Logger {
    void receive(const Sample& s) noexcept { ++count; COLLAPSE_WORK(s.value ^ count); }
    uint32_t count = 0;
};

collapse::Slot<Controller> controller;
collapse::Slot<Logger> logger;
}

COLLAPSE_ENTRY void collapse_setup() { controller.emplace(); logger.emplace(); }
COLLAPSE_ENTRY void collapse_publish(uint32_t v)
{
    const Sample s{collapse::arg(v)};
    controller->receive(s);
    logger->receive(s);
}
COLLAPSE_ENTRY void collapse_teardown() { logger.reset(); controller.reset(); }
