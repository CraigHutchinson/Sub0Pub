// COLLAPSE_REFERENCE: handwritten_runtime
/** Case: concrete transport endpoint. Pattern B1 as sub0_b1_wire, but ingress names the transport object itself
 *  as its origin (what a radio's receive callback has at hand), not its Forward adapter. Pins the fix for an echo:
 *  the transport and its adapter are the same origin, so ingress is not sent back out through the radio. */
#include "collapse_case.hpp"
#include "sub0pub/sub0pub.hpp"

namespace {
struct Sample { uint32_t value; };
struct Controller {
    void receive(const Sample& s) noexcept { COLLAPSE_WORK(s.value * 3U); }
};
struct Radio {
    void send(const Sample& s) noexcept { collapse::io(s.value); }
};
using Uplink = sub0::Forward<Radio>;
using Bus = sub0::Wiring<Controller, Uplink>;
collapse::Slot<Controller> controller;
collapse::Slot<Radio> radio;
collapse::Slot<Bus> bus;
}

COLLAPSE_ENTRY void collapse_setup()
{
    controller.emplace(); radio.emplace();
    Uplink uplink(radio.get()); // an adapter: the wiring holds it by value
    bus.emplace(controller.get(), uplink);
}
COLLAPSE_ENTRY void collapse_publish(uint32_t v)
{
    const Sample out{collapse::arg(v)};
    bus->publish(out);
    bus->publishFrom(radio.get(), Sample{out.value ^ 0x55U}); // ingress: origin is the radio
}
COLLAPSE_ENTRY void collapse_teardown() { bus.reset(); radio.reset(); controller.reset(); }
