/** Case: nested publication. Relay receives Sample, publishes Command{v + 100} and, for odd v, re-publishes
 *  Sample{v + 1} (same type, re-entrant, bounded by the data). Actuator receives Command, Tail (gain 5)
 *  receives Sample. Bound order relay, actuator, tail: the nested publications complete before Tail sees
 *  the outer Sample.
 *  Reference: direct calls; the nested publications are plain function calls. */
#include "collapse_case.hpp"

namespace {
struct Sample { uint32_t value; };
struct Command { uint32_t code; };
struct Actuator {
    void receive(const Command& c) noexcept { COLLAPSE_WORK(c.code << 1U); }
};
struct Tail {
    void receive(const Sample& s) noexcept { COLLAPSE_WORK(s.value * 5U); }
};
struct Relay {
    void receive(const Sample& s) noexcept;
};
collapse::Slot<Relay> relay;
collapse::Slot<Actuator> actuator;
collapse::Slot<Tail> tail;
void publishSample(const Sample& s) noexcept
{
    relay->receive(s);
    tail->receive(s);
}
void Relay::receive(const Sample& s) noexcept
{
    COLLAPSE_WORK(s.value);
    actuator->receive(Command{s.value + 100U});
    if ((s.value & 1U) != 0U)
        publishSample(Sample{s.value + 1U});
}
}

COLLAPSE_ENTRY void collapse_setup() { relay.emplace(); actuator.emplace(); tail.emplace(); }
COLLAPSE_ENTRY void collapse_publish(uint32_t v) { publishSample(Sample{collapse::arg(v)}); }
COLLAPSE_ENTRY void collapse_teardown() { tail.reset(); actuator.reset(); relay.reset(); }
