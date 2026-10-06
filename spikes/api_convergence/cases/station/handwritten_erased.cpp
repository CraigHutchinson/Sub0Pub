/** Case: two message types in one wiring. Controller handles Sample and Command, Logger only Sample,
 *  Actuator only Command. Each publication sends Sample{v} then Command{v ^ 0xF}; bound order
 *  controller, logger, actuator, so Sample reaches controller then logger, Command controller then actuator.
 *  Type-erased reference: a non-template publisher with one context + function pointer per message type into a
 *  node holding the receivers' addresses. Equal work for `Sink<T>` (B3, one Sink per type). */
#include "collapse_case.hpp"

namespace {
struct Sample { uint32_t value; };
struct Command { uint32_t code; };
struct Controller {
    void receive(const Sample& s) noexcept { COLLAPSE_WORK(s.value * 3U); }
    void receive(const Command& c) noexcept { COLLAPSE_WORK(c.code + 1U); }
};
struct Logger {
    void receive(const Sample& s) noexcept { ++count; COLLAPSE_WORK(s.value ^ count); }
    uint32_t count = 0;
};
struct Actuator {
    void receive(const Command& c) noexcept { COLLAPSE_WORK(c.code << 2U); }
};
collapse::Slot<Controller> controller;
collapse::Slot<Logger> logger;
collapse::Slot<Actuator> actuator;
struct Node {
    Controller* c;
    Logger* log;
    Actuator* act;
};
void deliverSample(const void* n, const Sample& s) noexcept
{
    const Node* node = static_cast<const Node*>(n);
    node->c->receive(s);
    node->log->receive(s);
}
void deliverCommand(const void* n, const Command& c) noexcept
{
    const Node* node = static_cast<const Node*>(n);
    node->c->receive(c);
    node->act->receive(c);
}
struct Sensor { // non-template publisher: context + function pointer per type
    const void* sampleCtx;
    void (*sampleFn)(const void*, const Sample&) noexcept;
    const void* commandCtx;
    void (*commandFn)(const void*, const Command&) noexcept;
    void send(uint32_t v) noexcept
    {
        sampleFn(sampleCtx, Sample{v});
        commandFn(commandCtx, Command{v ^ 0xFU});
    }
};
collapse::Slot<Node> node;
collapse::Slot<Sensor> sensor;
}

COLLAPSE_ENTRY void collapse_setup()
{
    controller.emplace(); logger.emplace(); actuator.emplace();
    node.emplace(Node{&controller.get(), &logger.get(), &actuator.get()});
    sensor.emplace(Sensor{&node.get(), &deliverSample, &node.get(), &deliverCommand});
}
COLLAPSE_ENTRY void collapse_publish(uint32_t v) { sensor->send(collapse::arg(v)); }
COLLAPSE_ENTRY void collapse_teardown() { sensor.reset(); node.reset(); actuator.reset(); logger.reset(); controller.reset(); }
