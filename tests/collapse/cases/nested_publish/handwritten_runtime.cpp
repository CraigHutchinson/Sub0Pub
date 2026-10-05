/** Case: nested publication. Relay receives Sample, publishes Command{v + 100} and, for odd v, re-publishes
 *  Sample{v + 1} (same type, re-entrant, bounded by the data). Actuator receives Command, Tail (gain 5)
 *  receives Sample. Bound order relay, actuator, tail: the nested publications complete before Tail sees
 *  the outer Sample.
 *  Runtime-bound reference: the relay reaches the receivers through addresses stored at setup (one node shared
 *  by the publisher and the relay). Equal work for B1. */
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
struct Relay;
struct Node {
    Relay* relay;
    Actuator* actuator;
    Tail* tail;
    void publish(const Sample& s) const noexcept;
    void publish(const Command& c) const noexcept { actuator->receive(c); }
};
struct Relay {
    const Node* out;
    void receive(const Sample& s) noexcept
    {
        COLLAPSE_WORK(s.value);
        out->publish(Command{s.value + 100U});
        if ((s.value & 1U) != 0U)
            out->publish(Sample{s.value + 1U});
    }
};
void Node::publish(const Sample& s) const noexcept
{
    relay->receive(s);
    tail->receive(s);
}
collapse::Slot<Node> node;
collapse::Slot<Relay> relay;
collapse::Slot<Actuator> actuator;
collapse::Slot<Tail> tail;
}

COLLAPSE_ENTRY void collapse_setup()
{
    relay.emplace(Relay{&node.get()}); actuator.emplace(); tail.emplace();
    node.emplace(Node{&relay.get(), &actuator.get(), &tail.get()});
}
COLLAPSE_ENTRY void collapse_publish(uint32_t v) { node->publish(Sample{collapse::arg(v)}); }
COLLAPSE_ENTRY void collapse_teardown() { node.reset(); tail.reset(); actuator.reset(); relay.reset(); }
