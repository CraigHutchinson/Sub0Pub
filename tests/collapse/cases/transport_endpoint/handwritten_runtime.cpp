/** Case: transport endpoint (egress to the radio, ingress delivered locally only). Runtime-bound reference: the same work, with receiver addresses stored at setup and called through,
 *  as a careful engineer writes it when addresses are only known at run time. Equal work for patterns whose
 *  bindings are runtime values (B1 `wire(...)`), which select it with `// COLLAPSE_REFERENCE: handwritten_runtime`. */
#include "collapse_case.hpp"

namespace {
struct Sample { uint32_t value; };
struct Controller {
    void receive(const Sample& s) noexcept { COLLAPSE_WORK(s.value * 3U); }
};
struct Radio {
    void send(const Sample& s) noexcept { collapse::io(s.value); }
};
collapse::Slot<Controller> controller;
collapse::Slot<Radio> radio;
struct Node {
    Controller* c;
    Radio* r;
};
collapse::Slot<Node> node;
}

COLLAPSE_ENTRY void collapse_setup() { controller.emplace(); radio.emplace(); node.emplace(Node{&controller.get(), &radio.get()}); }
COLLAPSE_ENTRY void collapse_publish(uint32_t v)
{
    const Sample out{collapse::arg(v)};
    node->c->receive(out);
    node->r->send(out);
    const Sample in{out.value ^ 0x55U};   // received from the peer
    node->c->receive(in);
}
COLLAPSE_ENTRY void collapse_teardown() { node.reset(); radio.reset(); controller.reset(); }
