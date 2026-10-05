#include "receivers.hpp"

namespace {
collapse::Slot<app::Controller> controllerA;
collapse::Slot<app::Controller> controllerB;
collapse::Slot<app::Logger> logger;
struct Node {
    app::Controller* a;
    app::Controller* b;
    app::Logger* log;
};
void deliverNode(const void* n, const app::Sample& s) noexcept
{
    const Node* node = static_cast<const Node*>(n);
    node->a->receive(s);
    node->b->receive(s);
    node->log->receive(s);
}
collapse::Slot<Node> node;
collapse::Slot<app::Sensor> sensor;
}

COLLAPSE_ENTRY void collapse_setup()
{
    controllerA.emplace(3U); controllerB.emplace(5U); logger.emplace();
    node.emplace(Node{&controllerA.get(), &controllerB.get(), &logger.get()});
    sensor.emplace(app::Sensor{&node.get(), &deliverNode});
}
COLLAPSE_ENTRY void collapse_publish(uint32_t v) { sensor->send(collapse::arg(v)); }
COLLAPSE_ENTRY void collapse_teardown() { sensor.reset(); node.reset(); logger.reset(); controllerB.reset(); controllerA.reset(); }
