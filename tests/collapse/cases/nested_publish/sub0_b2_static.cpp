/** Case: nested publication. Relay receives Sample, publishes Command{v + 100} and, for odd v, re-publishes
 *  Sample{v + 1} (same type, re-entrant, bounded by the data). Actuator receives Command, Tail (gain 5)
 *  receives Sample. Bound order relay, actuator, tail: the nested publications complete before Tail sees
 *  the outer Sample.
 *  Pattern B2: the relay publishes on the static wiring that binds it (its receive() is defined after the alias). */
#include "collapse_case.hpp"
#include "sub0pub/sub0pub.hpp"

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
using Bus = sub0::StaticWiring<&relay, &actuator, &tail>;
void Relay::receive(const Sample& s) noexcept
{
    COLLAPSE_WORK(s.value);
    Bus::publish(Command{s.value + 100U});
    if ((s.value & 1U) != 0U)
        Bus::publish(Sample{s.value + 1U});
}
}

COLLAPSE_ENTRY void collapse_setup() { relay.emplace(); actuator.emplace(); tail.emplace(); }
COLLAPSE_ENTRY void collapse_publish(uint32_t v) { Bus::publish(Sample{collapse::arg(v)}); }
COLLAPSE_ENTRY void collapse_teardown() { tail.reset(); actuator.reset(); relay.reset(); }
