/** Case: concrete transport endpoint. Dynamic alternative: the public runtime registry with a
 *  sub0::Route<Sample, RadioPort> endpoint binding (split horizon on ingress via Route::inject). */
#include "collapse_case.hpp"
#include "sub0pub/sub0pub.hpp"

namespace {
struct Sample { uint32_t value; using sub0_config = sub0::config<sub0::ThreadLocalContext>; }; // routes need a publish context: the documented opt-in
struct Controller final : sub0::Subscribe<Sample> {
    void receive(const Sample& s) noexcept override { COLLAPSE_WORK(s.value * 3U); }
};
struct Radio {
    void send(const Sample& s) noexcept { collapse::io(s.value); }
};
struct RadioPort {   // adapts Radio to the Transport concept
    explicit RadioPort(Radio& r) noexcept : radio(r) {}
    sub0::SendResult send(const Sample& s) noexcept { radio.send(s); return sub0::SendResult::Accepted; }
    Radio& radio;
};
struct Sensor : sub0::Publish<Sample> {
    void send(const Sample& s) noexcept { sub0::publish(*this, s); }
};
using Uplink = sub0::Route<Sample, RadioPort>;
collapse::Slot<Controller> controller;
collapse::Slot<Radio> radio;
collapse::Slot<RadioPort> port;
collapse::Slot<Uplink> uplink;
collapse::Slot<Sensor> sensor;
}

COLLAPSE_ENTRY void collapse_setup()
{
    controller.emplace(); radio.emplace(); port.emplace(radio.get()); uplink.emplace(port.get()); sensor.emplace();
}
COLLAPSE_ENTRY void collapse_publish(uint32_t v)
{
    const Sample out{collapse::arg(v)};
    sensor->send(out);
    uplink->inject(Sample{out.value ^ 0x55U});
}
COLLAPSE_ENTRY void collapse_teardown() { sensor.reset(); uplink.reset(); port.reset(); radio.reset(); controller.reset(); }
