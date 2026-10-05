// COLLAPSE_REFERENCE: handwritten_runtime
/** Case: concrete transport endpoint. Pattern B1: typed wiring with a typed Forward<Radio> endpoint binding;
 *  ingress via publishFrom(endpoint) skips the endpoint (split horizon). */
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
    bus->publishFrom<Uplink>(Sample{out.value ^ 0x55U}); // ingress: origin identified by its (unique) type
}
COLLAPSE_ENTRY void collapse_teardown() { bus.reset(); radio.reset(); controller.reset(); }
