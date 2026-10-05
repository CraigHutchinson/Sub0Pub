#include "receivers.hpp"

namespace {
collapse::Slot<app::Controller> controllerA;
collapse::Slot<app::Controller> controllerB;
collapse::Slot<app::Logger> logger;
struct Sensor {
    app::Controller* a;
    app::Controller* b;
    app::Logger* log;
    void send(uint32_t v) noexcept
    {
        const app::Sample s{v};
        a->receive(s);
        b->receive(s);
        log->receive(s);
    }
};
collapse::Slot<Sensor> sensor;
}

COLLAPSE_ENTRY void collapse_setup()
{
    controllerA.emplace(3U); controllerB.emplace(5U); logger.emplace();
    sensor.emplace(Sensor{&controllerA.get(), &controllerB.get(), &logger.get()});
}
COLLAPSE_ENTRY void collapse_publish(uint32_t v) { sensor->send(collapse::arg(v)); }
COLLAPSE_ENTRY void collapse_teardown() { sensor.reset(); logger.reset(); controllerB.reset(); controllerA.reset(); }
