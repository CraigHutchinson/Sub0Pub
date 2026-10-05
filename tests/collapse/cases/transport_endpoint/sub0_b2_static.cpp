/** Case: concrete transport endpoint. Pattern B2: static topology with a StaticForward<&radio> endpoint binding. */
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
collapse::Slot<Controller> controller;
collapse::Slot<Radio> radio;
using Uplink = sub0::StaticForward<&radio>;
Uplink uplink;
using Bus = sub0::StaticWiring<&controller, &uplink>;
}

COLLAPSE_ENTRY void collapse_setup() { controller.emplace(); radio.emplace(); }
COLLAPSE_ENTRY void collapse_publish(uint32_t v)
{
    const Sample out{collapse::arg(v)};
    Bus::publish(out);
    Bus::publishFrom(uplink, Sample{out.value ^ 0x55U});
}
COLLAPSE_ENTRY void collapse_teardown() { radio.reset(); controller.reset(); }
