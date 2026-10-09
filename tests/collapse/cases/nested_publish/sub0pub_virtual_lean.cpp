/** Case: nested publication. Relay receives Sample, publishes Command{v + 100} and, for odd v, re-publishes
 *  Sample{v + 1} (same type, re-entrant, bounded by the data). Actuator receives Command, Tail (gain 5)
 *  receives Sample. Bound order relay, actuator, tail: the nested publications complete before Tail sees
 *  the outer Sample.
 *  The runtime broker uses direct dispatch: nested publication is supported without changing the table. */
// Assertions are disabled for the lean measurement.
#define SUB0PUB_ASSERT false
#include "collapse_case.hpp"
#include "sub0pub/sub0pub.hpp"

namespace {
struct Sample { uint32_t value; };
struct Command { uint32_t code; };
struct Relay final : sub0::Subscribe<Sample>, sub0::Publish<Sample>, sub0::Publish<Command> {
    void receive(const Sample& s) noexcept override
    {
        COLLAPSE_WORK(s.value);
        sub0::publish(static_cast<sub0::Publish<Command>&>(*this), Command{s.value + 100U});
        if ((s.value & 1U) != 0U)
            sub0::publish(static_cast<sub0::Publish<Sample>&>(*this), Sample{s.value + 1U});
    }
};
struct Actuator final : sub0::Subscribe<Command> {
    void receive(const Command& c) noexcept override { COLLAPSE_WORK(c.code << 1U); }
};
struct Tail final : sub0::Subscribe<Sample> {
    void receive(const Sample& s) noexcept override { COLLAPSE_WORK(s.value * 5U); }
};
struct Sensor : sub0::Publish<Sample> {
    void send(uint32_t v) noexcept { sub0::publish(*this, Sample{v}); }
};
collapse::Slot<Sensor> sensor;
collapse::Slot<Relay> relay;
collapse::Slot<Actuator> actuator;
collapse::Slot<Tail> tail;
}

COLLAPSE_ENTRY void collapse_setup() { sensor.emplace(); relay.emplace(); actuator.emplace(); tail.emplace(); }
COLLAPSE_ENTRY void collapse_publish(uint32_t v) { sensor->send(collapse::arg(v)); }
COLLAPSE_ENTRY void collapse_teardown() { tail.reset(); actuator.reset(); relay.reset(); sensor.reset(); }
