// COLLAPSE_REFERENCE: handwritten_runtime
/** Publisher spelling: the CRTP publisher mixin sub0::Publisher<Derived, Out>. The user still writes
 *  `template<class Out>` and still names `Out` once (in the base-class argument list and the constructor
 *  forwarding call), but gets `this->publish(msg)`
 *  instead of `out.publish(msg)`, and the "does this publisher have an output" question is answered by the base
 *  class rather than a hand-rolled member. */
#include "collapse_case.hpp"
#include "sub0pub/sub0pub.hpp"

namespace {
struct Sample { uint32_t value; };
struct Controller {
    explicit Controller(uint32_t g) noexcept : gain(g) {}
    void receive(const Sample& s) noexcept { COLLAPSE_WORK(s.value * gain); }
    uint32_t gain;
};
struct Logger {
    void receive(const Sample& s) noexcept { ++count; COLLAPSE_WORK(s.value ^ count); }
    uint32_t count = 0;
};

// --- what the user writes ---
template<class Out>
struct Sensor : sub0::Publisher<Sensor<Out>, Out> {
    using sub0::Publisher<Sensor<Out>, Out>::Publisher;
    void send(uint32_t v) noexcept { this->publish(Sample{v}); }
};
// ---

using Bus = sub0::Wiring<Controller, Controller, Logger>;
collapse::Slot<Controller> controllerA;
collapse::Slot<Controller> controllerB;
collapse::Slot<Logger> logger;
collapse::Slot<Sensor<Bus>> sensor;
}

COLLAPSE_ENTRY void collapse_setup() { controllerA.emplace(3U); controllerB.emplace(5U); logger.emplace(); sensor.emplace(Bus(controllerA.get(), controllerB.get(), logger.get())); }
COLLAPSE_ENTRY void collapse_publish(uint32_t v) { sensor->send(collapse::arg(v)); }
COLLAPSE_ENTRY void collapse_teardown() { sensor.reset(); logger.reset(); controllerB.reset(); controllerA.reset(); }
