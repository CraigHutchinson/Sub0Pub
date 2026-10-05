/** Case: two links of one transport type. Each publication goes to a controller and out through radio A
 *  and radio B; a message then arrives on radio A (ingress) and goes to the controller and out through B
 *  only (split horizon between two endpoints of the same type).
 *  Pattern B2: StaticForward<&radioA> and StaticForward<&radioB> are distinct types, so naming radio A as the
 *  origin is decided at compile time. */
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
collapse::Slot<Controller> controller;
collapse::Slot<Radio> radioA;
collapse::Slot<Radio> radioB;
sub0::StaticForward<&radioA> linkA;
sub0::StaticForward<&radioB> linkB;
using Bus = sub0::StaticWiring<&controller, &linkA, &linkB>;
}

COLLAPSE_ENTRY void collapse_setup() { controller.emplace(); radioA.emplace(1U); radioB.emplace(2U); }
COLLAPSE_ENTRY void collapse_publish(uint32_t v)
{
    const Sample out{collapse::arg(v)};
    Bus::publish(out);
    Bus::publishFrom(linkA, Sample{out.value ^ 0x55U});
}
COLLAPSE_ENTRY void collapse_teardown() { radioB.reset(); radioA.reset(); controller.reset(); }
