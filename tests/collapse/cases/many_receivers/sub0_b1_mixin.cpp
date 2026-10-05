// COLLAPSE_REFERENCE: handwritten_runtime
/** Case: 32 receivers of one type. Pattern B1 through the public CRTP publisher mixin sub0::Publisher<Derived, Out> (the recommended
 *  publisher form when the topology is not known where the publisher is written); otherwise as sub0_b1_wire.
 *  templated on its output and holds the wiring (32 receiver references) by value. */
#include "collapse_case.hpp"
#include "sub0pub/sub0pub.hpp"
#include "many.hpp"

namespace {
struct Sample { uint32_t value; };
struct Controller {
    explicit Controller(uint32_t g) noexcept : gain(g) {}
    void receive(const Sample& s) noexcept { COLLAPSE_WORK(s.value * gain); }
    uint32_t gain;
};
#define MANY_DECLARE(i) collapse::Slot<Controller> c##i;
MANY_EACH(MANY_DECLARE)
template<class Out>
struct Sensor : sub0::Publisher<Sensor<Out>, Out> { // the recommended mixin: holds the wiring by value
    using sub0::Publisher<Sensor<Out>, Out>::Publisher;
    void send(uint32_t v) noexcept { this->publish(Sample{v}); }
};
using Bus = sub0::Wiring<MANY_TYPES>;
collapse::Slot<Sensor<Bus>> sensor;
}

#define MANY_EMPLACE(i) c##i.emplace(i + 1U);
#define MANY_RESET(i) c##i.reset();
COLLAPSE_ENTRY void collapse_setup() { MANY_EACH(MANY_EMPLACE) sensor.emplace(Bus(MANY_GETS)); }
COLLAPSE_ENTRY void collapse_publish(uint32_t v) { sensor->send(collapse::arg(v)); }
COLLAPSE_ENTRY void collapse_teardown() { sensor.reset(); MANY_EACH(MANY_RESET) }
