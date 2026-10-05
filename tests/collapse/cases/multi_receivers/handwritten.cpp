/** Case: multiple receivers including a repeated type. Reference: direct calls in subscription order
 *  (controller A, controller B, logger). Instance selection and order are part of the checksum. */
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
collapse::Slot<Controller> controllerB;
collapse::Slot<Logger> logger;
}

COLLAPSE_ENTRY void collapse_setup() { controllerA.emplace(3U); controllerB.emplace(5U); logger.emplace(); }
COLLAPSE_ENTRY void collapse_publish(uint32_t v)
{
    const Sample s{collapse::arg(v)};
    controllerA->receive(s);
    controllerB->receive(s);
    logger->receive(s);
}
COLLAPSE_ENTRY void collapse_teardown() { logger.reset(); controllerB.reset(); controllerA.reset(); }
