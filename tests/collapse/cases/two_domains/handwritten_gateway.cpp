/** Case: two independent domains, published by ONE publisher (a gateway feeding two sessions). Runtime-bound
 *  reference with the same layout as sub0_b1_one_publisher: one object holding the addresses of both domains'
 *  receivers (stored at setup). Equal work for that variant, which selects it with
 *  `// COLLAPSE_REFERENCE: handwritten_gateway`; `handwritten_runtime` (two publisher objects) is laid out differently. */
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
struct Gateway {
    Controller* a;
    Logger* logA;
    Controller* b;
    void send(uint32_t va, uint32_t vb) noexcept
    {
        const Sample sa{va};
        a->receive(sa);
        logA->receive(sa);
        b->receive(Sample{vb});
    }
};
collapse::Slot<Controller> controllerA;
collapse::Slot<Logger> loggerA;
collapse::Slot<Controller> controllerB;
collapse::Slot<Gateway> gateway;
}

COLLAPSE_ENTRY void collapse_setup()
{
    controllerA.emplace(3U); loggerA.emplace(); controllerB.emplace(5U);
    gateway.emplace(Gateway{&controllerA.get(), &loggerA.get(), &controllerB.get()});
}
COLLAPSE_ENTRY void collapse_publish(uint32_t v)
{
    const uint32_t a = collapse::arg(v);
    gateway->send(a, collapse::arg(v + 1U));
}
COLLAPSE_ENTRY void collapse_teardown() { gateway.reset(); controllerB.reset(); loggerA.reset(); controllerA.reset(); }
