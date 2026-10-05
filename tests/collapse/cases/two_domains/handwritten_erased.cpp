/** Case: two independent domains. Type-erased reference: one non-template publisher per domain, each a context +
 *  function pointer into a node holding that domain's receivers. Equal work for B3. */
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
struct NodeA {
    Controller* c;
    Logger* log;
};
struct NodeB {
    Controller* c;
};
void deliverA(const void* n, const Sample& s) noexcept
{
    const NodeA* node = static_cast<const NodeA*>(n);
    node->c->receive(s);
    node->log->receive(s);
}
void deliverB(const void* n, const Sample& s) noexcept { static_cast<const NodeB*>(n)->c->receive(s); }
struct Sensor { // non-template publisher: context + function pointer
    const void* ctx;
    void (*fn)(const void*, const Sample&) noexcept;
    void send(uint32_t v) noexcept { fn(ctx, Sample{v}); }
};
collapse::Slot<Controller> controllerA;
collapse::Slot<Logger> loggerA;
collapse::Slot<Controller> controllerB;
collapse::Slot<NodeA> nodeA;
collapse::Slot<NodeB> nodeB;
collapse::Slot<Sensor> sensorA;
collapse::Slot<Sensor> sensorB;
}

COLLAPSE_ENTRY void collapse_setup()
{
    controllerA.emplace(3U); loggerA.emplace(); controllerB.emplace(5U);
    nodeA.emplace(NodeA{&controllerA.get(), &loggerA.get()});
    nodeB.emplace(NodeB{&controllerB.get()});
    sensorA.emplace(Sensor{&nodeA.get(), &deliverA});
    sensorB.emplace(Sensor{&nodeB.get(), &deliverB});
}
COLLAPSE_ENTRY void collapse_publish(uint32_t v)
{
    sensorA->send(collapse::arg(v));
    sensorB->send(collapse::arg(v + 1U));
}
COLLAPSE_ENTRY void collapse_teardown()
{
    sensorB.reset(); sensorA.reset(); nodeB.reset(); nodeA.reset();
    controllerB.reset(); loggerA.reset(); controllerA.reset();
}
