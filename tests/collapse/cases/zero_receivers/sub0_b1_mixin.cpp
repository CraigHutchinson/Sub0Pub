/** Case: zero receivers.
 *  Pattern B1 through the public CRTP publisher mixin sub0::Publisher<Derived, Out> (the recommended
 *  publisher form when the topology is not known where the publisher is written); otherwise as sub0_b1_wire. */
#include "collapse_case.hpp"
#include "sub0pub/sub0pub.hpp"

namespace {
struct Sample { uint32_t value; };

template<class Out>
struct Sensor : sub0::Publisher<Sensor<Out>, Out> { // the recommended mixin: holds the wiring by value
    using sub0::Publisher<Sensor<Out>, Out>::Publisher;
    void send(uint32_t v) noexcept { this->publish(Sample{v}); }
};
using Bus = sub0::Wiring<>;
collapse::Slot<Sensor<Bus>> sensor;
}

COLLAPSE_ENTRY void collapse_setup() { sensor.emplace(Bus()); }
COLLAPSE_ENTRY void collapse_publish(uint32_t v) { sensor->send(collapse::arg(v)); }
COLLAPSE_ENTRY void collapse_teardown() { sensor.reset(); }
