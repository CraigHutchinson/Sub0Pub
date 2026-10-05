/** Case: filters (controller always, monitor only for even values). Type-erased reference: a non-template publisher written by hand the C way -- a context pointer to a node
 *  holding the receivers' addresses (stored at setup) plus a function pointer that delivers to them. Equal work for
 *  `Sink<T>` (B3), which selects it with `// COLLAPSE_REFERENCE: handwritten_erased`. */
#include "collapse_case.hpp"

namespace {
struct Sample { uint32_t value; };
struct Controller {
    void receive(const Sample& s) noexcept { COLLAPSE_WORK(s.value * 3U); }
};
struct EvenMonitor {
    void receive(const Sample& s) noexcept { COLLAPSE_WORK(s.value + 7U); }
};
collapse::Slot<Controller> controller;
collapse::Slot<EvenMonitor> monitor;
struct Node {
    Controller* c;
    EvenMonitor* m;
    void deliver(const Sample& s) const noexcept
    {
        c->receive(s);
        if ((s.value & 1U) == 0U)
            m->receive(s);
    }
};
struct Sensor { // non-template publisher: context + function pointer
    const void* ctx;
    void (*fn)(const void*, const Sample&) noexcept;
    void send(uint32_t v) noexcept { fn(ctx, Sample{v}); }
};
collapse::Slot<Node> node;
collapse::Slot<Sensor> sensor;
void deliverNode(const void* c, const Sample& s) noexcept { static_cast<const Node*>(c)->deliver(s); }
}

COLLAPSE_ENTRY void collapse_setup() { controller.emplace(); monitor.emplace(); node.emplace(Node{&controller.get(), &monitor.get()}); sensor.emplace(Sensor{&node.get(), &deliverNode}); }
COLLAPSE_ENTRY void collapse_publish(uint32_t v) { sensor->send(collapse::arg(v)); }
COLLAPSE_ENTRY void collapse_teardown() { sensor.reset(); node.reset(); monitor.reset(); controller.reset(); }
