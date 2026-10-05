// COLLAPSE_REFERENCE: handwritten_erased
/** Case: one concrete receiver.
 *  Pattern B3: non-template publisher holding a type-erased Sink<Sample> into typed wiring (public API, include/sub0pub/sub0pub.hpp). */
#include "collapse_case.hpp"
#include "sub0pub/sub0pub.hpp"

namespace {
struct Sample { uint32_t value; };
struct Controller {
    explicit Controller(uint32_t g) noexcept : gain(g) {}
    void receive(const Sample& s) noexcept { COLLAPSE_WORK(s.value * gain); }
    uint32_t gain;
};

struct Sensor {
    explicit Sensor(sub0::Sink<Sample> o) noexcept : out(o) {}
    void send(uint32_t v) noexcept { out.publish(Sample{v}); }
    sub0::Sink<Sample> out;
};
using Bus = sub0::Wiring<Controller>;
collapse::Slot<Controller> controller;
collapse::Slot<Bus> bus;
collapse::Slot<Sensor> sensor;
}

COLLAPSE_ENTRY void collapse_setup() { controller.emplace(3U); bus.emplace(controller.get()); sensor.emplace(sub0::Sink<Sample>(bus.get())); }
COLLAPSE_ENTRY void collapse_publish(uint32_t v) { sensor->send(collapse::arg(v)); }
COLLAPSE_ENTRY void collapse_teardown() { sensor.reset(); bus.reset(); controller.reset(); }
