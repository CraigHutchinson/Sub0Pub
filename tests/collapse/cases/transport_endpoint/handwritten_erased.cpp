/** Case: concrete transport endpoint. Type-erased reference: a non-template publisher (context + function pointer)
 *  sends egress through a node holding the controller's and radio's addresses; ingress, handled where the
 *  application is composed, reaches the controller through the same node. Equal work for B3. */
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
void deliverEgress(const void* n, const Sample& s) noexcept
{
    const Node* node = static_cast<const Node*>(n);
    node->c->receive(s);
    node->r->send(s);
}
struct Sensor { // non-template publisher: context + function pointer
    const void* ctx;
    void (*fn)(const void*, const Sample&) noexcept;
    void send(const Sample& s) noexcept { fn(ctx, s); }
};
collapse::Slot<Node> node;
collapse::Slot<Sensor> sensor;
}

COLLAPSE_ENTRY void collapse_setup()
{
    controller.emplace(); radio.emplace();
    node.emplace(Node{&controller.get(), &radio.get()});
    sensor.emplace(Sensor{&node.get(), &deliverEgress});
}
COLLAPSE_ENTRY void collapse_publish(uint32_t v)
{
    const Sample out{collapse::arg(v)};
    sensor->send(out);
    node->c->receive(Sample{out.value ^ 0x55U}); // ingress: everything but the radio
}
COLLAPSE_ENTRY void collapse_teardown() { sensor.reset(); node.reset(); radio.reset(); controller.reset(); }
