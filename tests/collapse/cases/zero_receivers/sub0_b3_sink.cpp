// COLLAPSE_REFERENCE: handwritten_erased
/** Case: zero receivers.
 *  Pattern B3: non-template publisher holding a type-erased Sink<Sample> into typed wiring (public API, include/sub0pub/sub0pub.hpp). */
#include "collapse_case.hpp"
#include "sub0pub/sub0pub.hpp"

namespace {
struct Sample { uint32_t value; };

struct Sensor {
    explicit Sensor(sub0::Sink<Sample> o) noexcept : out(o) {}
    void send(uint32_t v) noexcept { out.publish(Sample{v}); }
    sub0::Sink<Sample> out;
};
using Bus = sub0::Wiring<>;
collapse::Slot<Bus> bus;
collapse::Slot<Sensor> sensor;
}

COLLAPSE_ENTRY void collapse_setup() { bus.emplace(); sensor.emplace(sub0::Sink<Sample>(bus.get())); }
COLLAPSE_ENTRY void collapse_publish(uint32_t v) { sensor->send(collapse::arg(v)); }
COLLAPSE_ENTRY void collapse_teardown() { sensor.reset(); bus.reset(); }
