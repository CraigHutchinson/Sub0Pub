// COLLAPSE_REFERENCE: handwritten_runtime
/** Case: nested publication. Relay receives Sample, publishes Command{v + 100} and, for odd v, re-publishes
 *  Sample{v + 1} (same type, re-entrant, bounded by the data). Actuator receives Command, Tail (gain 5)
 *  receives Sample. Bound order relay, actuator, tail: the nested publications complete before Tail sees
 *  the outer Sample.
 *  Pattern B1: the relay holds a pointer to the runtime wiring that binds it (declared before the relay is
 *  complete: Wiring only names its bound types). */
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
struct Relay;
using Bus = sub0::Wiring<Relay, Actuator, Tail>;
struct Relay {
    const Bus* out;
    void receive(const Sample& s) noexcept
    {
        COLLAPSE_WORK(s.value);
        out->publish(Command{s.value + 100U});
        if ((s.value & 1U) != 0U)
            out->publish(Sample{s.value + 1U});
    }
};
collapse::Slot<Bus> bus;
collapse::Slot<Relay> relay;
collapse::Slot<Actuator> actuator;
collapse::Slot<Tail> tail;
}

COLLAPSE_ENTRY void collapse_setup()
{
    relay.emplace(Relay{&bus.get()}); actuator.emplace(); tail.emplace();
    bus.emplace(relay.get(), actuator.get(), tail.get());
}
COLLAPSE_ENTRY void collapse_publish(uint32_t v) { bus->publish(Sample{collapse::arg(v)}); }
COLLAPSE_ENTRY void collapse_teardown() { bus.reset(); tail.reset(); actuator.reset(); relay.reset(); }
