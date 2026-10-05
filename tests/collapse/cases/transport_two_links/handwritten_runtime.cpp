/** Case: two links of one transport type. Each publication goes to a controller and out through radio A
 *  and radio B; a message then arrives on radio A (ingress) and goes to the controller and out through B
 *  only (split horizon between two endpoints of the same type).
 *  Runtime-bound reference: addresses stored at setup; the ingress handler of link A is written knowing it is
 *  link A, so it needs no comparison. Equal work for B1. */
#include "collapse_case.hpp"

namespace {
struct Sample { uint32_t value; };
struct Controller {
    void receive(const Sample& s) noexcept { COLLAPSE_WORK(s.value * 3U); }
};
struct Radio {
    explicit Radio(uint32_t c) noexcept : channel(c) {}
    void send(const Sample& s) noexcept { collapse::io(s.value + channel); }
    uint32_t channel;
};
collapse::Slot<Controller> controller;
collapse::Slot<Radio> radioA;
collapse::Slot<Radio> radioB;
struct Node {
    Controller* c;
    Radio* a;
    Radio* b;
};
collapse::Slot<Node> node;
}

COLLAPSE_ENTRY void collapse_setup()
{
    controller.emplace(); radioA.emplace(1U); radioB.emplace(2U);
    node.emplace(Node{&controller.get(), &radioA.get(), &radioB.get()});
}
COLLAPSE_ENTRY void collapse_publish(uint32_t v)
{
    const Sample out{collapse::arg(v)};
    node->c->receive(out);
    node->a->send(out);
    node->b->send(out);
    const Sample in{out.value ^ 0x55U};
    node->c->receive(in);
    node->b->send(in);
}
COLLAPSE_ENTRY void collapse_teardown() { node.reset(); radioB.reset(); radioA.reset(); controller.reset(); }
