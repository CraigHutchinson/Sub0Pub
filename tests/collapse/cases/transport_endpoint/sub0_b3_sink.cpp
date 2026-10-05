// COLLAPSE_REFERENCE: handwritten_erased
/** Case: concrete transport endpoint. Pattern B3: a non-template publisher holds a Sink<Sample> into a typed
 *  wiring with a Forward<Radio> endpoint; ingress, handled at the composition point, uses publishFrom. */
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
struct Sensor {
    explicit Sensor(sub0::Sink<Sample> o) noexcept : out(o) {}
    void send(const Sample& s) noexcept { out.publish(s); }
    sub0::Sink<Sample> out;
};
collapse::Slot<Controller> controller;
collapse::Slot<Radio> radio;
collapse::Slot<Bus> bus;
collapse::Slot<Sensor> sensor;
}

COLLAPSE_ENTRY void collapse_setup()
{
    controller.emplace(); radio.emplace();
    Uplink uplink(radio.get()); // an adapter: the wiring holds it by value
    bus.emplace(controller.get(), uplink);
    sensor.emplace(sub0::Sink<Sample>(bus.get()));
}
COLLAPSE_ENTRY void collapse_publish(uint32_t v)
{
    const Sample out{collapse::arg(v)};
    sensor->send(out);
    bus->publishFrom(radio.get(), Sample{out.value ^ 0x55U});
}
COLLAPSE_ENTRY void collapse_teardown() { sensor.reset(); bus.reset(); radio.reset(); controller.reset(); }
