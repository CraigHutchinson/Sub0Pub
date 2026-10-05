// COLLAPSE_REFERENCE: handwritten_runtime
/** Case: two links of one transport type. Each publication goes to a controller and out through radio A
 *  and radio B; a message then arrives on radio A (ingress) and goes to the controller and out through B
 *  only (split horizon between two endpoints of the same type).
 *  Pattern B1, natural form: two Forward<Radio> bindings of one type; ingress names radio A, so each Forward is
 *  compared with it at run time (known issue K15's alternative: an address compare per same-typed endpoint). */
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
using Link = sub0::Forward<Radio>;
using Bus = sub0::Wiring<Controller, Link, Link>;
collapse::Slot<Controller> controller;
collapse::Slot<Radio> radioA;
collapse::Slot<Radio> radioB;
collapse::Slot<Bus> bus;
}

COLLAPSE_ENTRY void collapse_setup()
{
    controller.emplace(); radioA.emplace(1U); radioB.emplace(2U);
    Link linkA(radioA.get()); // adapters: the wiring holds them by value
    Link linkB(radioB.get());
    bus.emplace(controller.get(), linkA, linkB);
}
COLLAPSE_ENTRY void collapse_publish(uint32_t v)
{
    const Sample out{collapse::arg(v)};
    bus->publish(out);
    bus->publishFrom(radioA.get(), Sample{out.value ^ 0x55U});
}
COLLAPSE_ENTRY void collapse_teardown() { bus.reset(); radioB.reset(); radioA.reset(); controller.reset(); }
