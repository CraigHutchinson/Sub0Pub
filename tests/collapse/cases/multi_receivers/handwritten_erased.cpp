/** Case: multiple receivers including a repeated type. Type-erased reference: a non-template publisher written by hand the C way -- a context pointer to a node
 *  holding the receivers' addresses (stored at setup) plus a function pointer that delivers to them. Equal work for
 *  `Sink<T>` (B3), which selects it with `// COLLAPSE_REFERENCE: handwritten_erased`. */
#include "collapse_case.hpp"

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
collapse::Slot<Controller> controllerA;
collapse::Slot<Controller> controllerB;
collapse::Slot<Logger> logger;
struct Node {
    Controller* a;
    Controller* b;
    Logger* log;
    void deliver(const Sample& s) const noexcept { a->receive(s); b->receive(s); log->receive(s); }
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

COLLAPSE_ENTRY void collapse_setup() { controllerA.emplace(3U); controllerB.emplace(5U); logger.emplace(); node.emplace(Node{&controllerA.get(), &controllerB.get(), &logger.get()}); sensor.emplace(Sensor{&node.get(), &deliverNode}); }
COLLAPSE_ENTRY void collapse_publish(uint32_t v) { sensor->send(collapse::arg(v)); }
COLLAPSE_ENTRY void collapse_teardown() { sensor.reset(); node.reset(); logger.reset(); controllerB.reset(); controllerA.reset(); }
