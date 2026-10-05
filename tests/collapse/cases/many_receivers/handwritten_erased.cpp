/** Case: 32 receivers of one type. Type-erased reference: a non-template publisher written by hand the C way --
 *  a context pointer to a node holding the receivers' addresses (stored at setup) plus a function pointer that
 *  delivers to them in order. Equal work for `Sink<T>` (B3), which selects it with
 *  `// COLLAPSE_REFERENCE: handwritten_erased`. */
#include "collapse_case.hpp"
#include "many.hpp"

namespace {
struct Sample { uint32_t value; };
struct Controller {
    explicit Controller(uint32_t g) noexcept : gain(g) {}
    void receive(const Sample& s) noexcept { COLLAPSE_WORK(s.value * gain); }
    uint32_t gain;
};
#define MANY_DECLARE(i) collapse::Slot<Controller> c##i;
MANY_EACH(MANY_DECLARE)
struct Node {
    Controller* targets[MANY_COUNT];
    void deliver(const Sample& s) const noexcept
    {
#define MANY_CALL(i) targets[i]->receive(s);
        MANY_EACH(MANY_CALL)
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

#define MANY_EMPLACE(i) c##i.emplace(i + 1U);
#define MANY_RESET(i) c##i.reset();
#define MANY_ADDRESS_OF(i) &c##i.get(),
COLLAPSE_ENTRY void collapse_setup() { MANY_EACH(MANY_EMPLACE) node.emplace(Node{{MANY_EACH(MANY_ADDRESS_OF)}}); sensor.emplace(Sensor{&node.get(), &deliverNode}); }
COLLAPSE_ENTRY void collapse_publish(uint32_t v) { sensor->send(collapse::arg(v)); }
COLLAPSE_ENTRY void collapse_teardown() { sensor.reset(); node.reset(); MANY_EACH(MANY_RESET) }
