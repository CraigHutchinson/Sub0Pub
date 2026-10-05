#include "receivers.hpp"

namespace {
collapse::Slot<app::Controller> controllerA;
collapse::Slot<app::Controller> controllerB;
collapse::Slot<app::Logger> logger;
}

COLLAPSE_ENTRY void collapse_setup() { controllerA.emplace(3U); controllerB.emplace(5U); logger.emplace(); }
COLLAPSE_ENTRY void collapse_publish(uint32_t v)
{
    const app::Sample s{collapse::arg(v)};
    controllerA->receive(s);
    controllerB->receive(s);
    logger->receive(s);
}
COLLAPSE_ENTRY void collapse_teardown() { logger.reset(); controllerB.reset(); controllerA.reset(); }
