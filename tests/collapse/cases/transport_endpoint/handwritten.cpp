/** Case: concrete transport endpoint. Each publication is delivered to a local controller and sent out through a
 *  radio (egress: required I/O, observable in both forms); a message then arrives from the remote peer (ingress)
 *  and is delivered locally but not sent back out (split horizon). Reference: direct calls.
 *  Today's sub0pub.hpp API has no split horizon (ingress would echo back out): not expressible equivalently. */
#include "collapse_case.hpp"

namespace {
struct Sample { uint32_t value; };
struct Controller {
    void receive(const Sample& s) noexcept { COLLAPSE_WORK(s.value * 3U); }
};
struct Radio {
    void send(const Sample& s) noexcept { collapse::io(s.value); }
};
collapse::Slot<Controller> controller;
collapse::Slot<Radio> radio;
}

COLLAPSE_ENTRY void collapse_setup() { controller.emplace(); radio.emplace(); }
COLLAPSE_ENTRY void collapse_publish(uint32_t v)
{
    const Sample out{collapse::arg(v)};
    controller->receive(out);
    radio->send(out);
    const Sample in{out.value ^ 0x55U};   // received from the peer
    controller->receive(in);
}
COLLAPSE_ENTRY void collapse_teardown() { radio.reset(); controller.reset(); }
