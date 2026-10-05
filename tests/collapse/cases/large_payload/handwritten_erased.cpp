/** Case: a large payload (64-byte Frame, 16 words, word i = v + i) delivered to a controller (reads words 0
 *  and 15) and a logger (xors word 7 with its count), by const reference.
 *  Type-erased reference: context + function pointer into a node holding the receivers' addresses. Equal work
 *  for B3. */
#include "collapse_case.hpp"

namespace {
struct Frame { uint32_t words[16]; };
inline Frame makeFrame(uint32_t v) noexcept
{
    Frame f;
    for (uint32_t i = 0; i < 16U; ++i)
        f.words[i] = v + i;
    return f;
}
struct Controller {
    void receive(const Frame& f) noexcept { COLLAPSE_WORK(f.words[0] * 3U + f.words[15]); }
};
struct Logger {
    void receive(const Frame& f) noexcept { ++count; COLLAPSE_WORK(f.words[7] ^ count); }
    uint32_t count = 0;
};
collapse::Slot<Controller> controller;
collapse::Slot<Logger> logger;
struct Node {
    Controller* c;
    Logger* log;
};
void deliverNode(const void* n, const Frame& f) noexcept
{
    const Node* node = static_cast<const Node*>(n);
    node->c->receive(f);
    node->log->receive(f);
}
struct Sensor {
    const void* ctx;
    void (*fn)(const void*, const Frame&) noexcept;
    void send(uint32_t v) noexcept { fn(ctx, makeFrame(v)); }
};
collapse::Slot<Node> node;
collapse::Slot<Sensor> sensor;
}

COLLAPSE_ENTRY void collapse_setup() { controller.emplace(); logger.emplace(); node.emplace(Node{&controller.get(), &logger.get()}); sensor.emplace(Sensor{&node.get(), &deliverNode}); }
COLLAPSE_ENTRY void collapse_publish(uint32_t v) { sensor->send(collapse::arg(v)); }
COLLAPSE_ENTRY void collapse_teardown() { sensor.reset(); node.reset(); logger.reset(); controller.reset(); }
