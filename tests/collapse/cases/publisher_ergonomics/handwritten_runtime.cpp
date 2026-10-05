/** Case: publisher spelling. Runtime-bound reference: the publisher holds its receivers' addresses, stored at
 *  setup, and calls through them in order (controller A, controller B, logger). Equal work for a publisher that
 *  binds its output at run time. */
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
struct Sensor {
    Controller* a;
    Controller* b;
    Logger* log;
    void send(uint32_t v) noexcept
    {
        const Sample s{v};
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
