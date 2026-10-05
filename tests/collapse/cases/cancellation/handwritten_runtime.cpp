/** Case: receiver-controlled early stop (see handwritten.cpp). Runtime-bound reference: the publisher holds the
 *  receivers' addresses (stored at setup) and returns early on the gate's decision. Equal work for B1. */
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
    bool receive(const Sample& s) noexcept // true: stop this publication here
    {
        COLLAPSE_WORK(s.value);
        return (s.value % 3U) == 0U;
    }
};
collapse::Slot<Gate> gate;
collapse::Slot<Controller> controller;
collapse::Slot<Logger> logger;
struct Sensor {
    Gate* g;
    Controller* c;
    Logger* log;
    void send(uint32_t v) noexcept
    {
        const Sample s{v};
        if (g->receive(s))
            return;
        c->receive(s);
        log->receive(s);
    }
};
collapse::Slot<Sensor> sensor;
}

COLLAPSE_ENTRY void collapse_setup()
{
    gate.emplace(); controller.emplace(3U); logger.emplace();
    sensor.emplace(Sensor{&gate.get(), &controller.get(), &logger.get()});
}
COLLAPSE_ENTRY void collapse_publish(uint32_t v) { sensor->send(collapse::arg(v)); }
COLLAPSE_ENTRY void collapse_teardown() { sensor.reset(); logger.reset(); controller.reset(); gate.reset(); }
