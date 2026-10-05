/** Case: two links of one transport type. Each publication goes to a controller and out through radio A
 *  and radio B; a message then arrives on radio A (ingress) and goes to the controller and out through B
 *  only (split horizon between two endpoints of the same type).
 *  Reference: direct calls. */
#include "collapse_case.hpp"

namespace {
struct Sample { uint32_t value; };
struct Controller {
    void receive(const Sample& s) noexcept { COLLAPSE_WORK(s.value * 3U); }
};
struct Radio {
    explicit Radio(uint32_t c) noexcept : channel(c) {}
    void send(const Sample& s) noexcept { collapse::io(s.value + channel); }
    uint32_t channel;
};
collapse::Slot<Controller> controller;
collapse::Slot<Radio> radioA;
collapse::Slot<Radio> radioB;
}

COLLAPSE_ENTRY void collapse_setup() { controller.emplace(); radioA.emplace(1U); radioB.emplace(2U); }
COLLAPSE_ENTRY void collapse_publish(uint32_t v)
{
    const Sample out{collapse::arg(v)};
    controller->receive(out);
    radioA->send(out);
    radioB->send(out);
    const Sample in{out.value ^ 0x55U}; // arrived on radio A
    controller->receive(in);
    radioB->send(in);
}
COLLAPSE_ENTRY void collapse_teardown() { radioB.reset(); radioA.reset(); controller.reset(); }
