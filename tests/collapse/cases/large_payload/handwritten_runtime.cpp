/** Case: a large payload (64-byte Frame, 16 words, word i = v + i) delivered to a controller (reads words 0
 *  and 15) and a logger (xors word 7 with its count), by const reference.
 *  Runtime-bound reference: receiver addresses stored at setup. Equal work for B1. */
#include "collapse_case.hpp"

namespace {
struct Frame { uint32_t words[16]; };
inline Frame makeFrame(uint32_t v) noexcept
{
    Frame f;
    for (uint32_t i = 0; i < 16U; ++i)
        f.words[i] = v + i;
    return f;
}
struct Controller {
    void receive(const Frame& f) noexcept { COLLAPSE_WORK(f.words[0] * 3U + f.words[15]); }
};
struct Logger {
    void receive(const Frame& f) noexcept { ++count; COLLAPSE_WORK(f.words[7] ^ count); }
    uint32_t count = 0;
};
collapse::Slot<Controller> controller;
collapse::Slot<Logger> logger;
struct Sensor {
    Controller* c;
    Logger* log;
    void send(uint32_t v) noexcept
    {
        const Frame f = makeFrame(v);
        c->receive(f);
        log->receive(f);
    }
};
collapse::Slot<Sensor> sensor;
}

COLLAPSE_ENTRY void collapse_setup() { controller.emplace(); logger.emplace(); sensor.emplace(Sensor{&controller.get(), &logger.get()}); }
COLLAPSE_ENTRY void collapse_publish(uint32_t v) { sensor->send(collapse::arg(v)); }
COLLAPSE_ENTRY void collapse_teardown() { sensor.reset(); logger.reset(); controller.reset(); }
