// COLLAPSE_REFERENCE: handwritten_runtime
/** Case: two links of one transport type. Each publication goes to a controller and out through radio A
 *  and radio B; a message then arrives on radio A (ingress) and goes to the controller and out through B
 *  only (split horizon between two endpoints of the same type).
 *  Pattern B1, best form: each link gets its own adapter type (a one-line subclass of Forward<Radio>), so
 *  publishFrom<LinkA>(msg) identifies the origin at compile time, with no compare. */
#include "collapse_case.hpp"
#include "sub0pub/sub0pub.hpp"

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
struct LinkA : sub0::Forward<Radio> { using Forward::Forward; };
struct LinkB : sub0::Forward<Radio> { using Forward::Forward; };
using Bus = sub0::Wiring<Controller, LinkA, LinkB>;
collapse::Slot<Controller> controller;
collapse::Slot<Radio> radioA;
collapse::Slot<Radio> radioB;
collapse::Slot<Bus> bus;
}

COLLAPSE_ENTRY void collapse_setup()
{
    controller.emplace(); radioA.emplace(1U); radioB.emplace(2U);
    LinkA linkA(radioA.get()); // adapters: the wiring holds them by value
    LinkB linkB(radioB.get());
    bus.emplace(controller.get(), linkA, linkB);
}
COLLAPSE_ENTRY void collapse_publish(uint32_t v)
{
    const Sample out{collapse::arg(v)};
    bus->publish(out);
    bus->publishFrom<LinkA>(Sample{out.value ^ 0x55U});
}
COLLAPSE_ENTRY void collapse_teardown() { bus.reset(); radioB.reset(); radioA.reset(); controller.reset(); }
