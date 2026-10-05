/** Case: a large payload (64-byte Frame, 16 words, word i = v + i) delivered to a controller (reads words 0
 *  and 15) and a logger (xors word 7 with its count), by const reference.
 *  The runtime broker at its leanest macros. */
// Today's API at its leanest settings (fair comparison): direct dispatch, no assertion checks
#define SUB0PUB_REENTRANT_SAFE false
#define SUB0PUB_ASSERT false
#include "collapse_case.hpp"
#include "sub0pub/sub0pub.hpp"

namespace {
struct Frame { uint32_t words[16]; };
inline Frame makeFrame(uint32_t v) noexcept
{
    Frame f;
    for (uint32_t i = 0; i < 16U; ++i)
        f.words[i] = v + i;
    return f;
}
struct Controller final : sub0::Subscribe<Frame> {
    void receive(const Frame& f) noexcept override { COLLAPSE_WORK(f.words[0] * 3U + f.words[15]); }
};
struct Logger final : sub0::Subscribe<Frame> {
    void receive(const Frame& f) noexcept override { ++count; COLLAPSE_WORK(f.words[7] ^ count); }
    uint32_t count = 0;
};
struct Sensor : sub0::Publish<Frame> {
    void send(uint32_t v) noexcept { sub0::publish(*this, makeFrame(v)); }
};
collapse::Slot<Sensor> sensor;
collapse::Slot<Controller> controller;
collapse::Slot<Logger> logger;
}

COLLAPSE_ENTRY void collapse_setup() { sensor.emplace(); controller.emplace(); logger.emplace(); }
COLLAPSE_ENTRY void collapse_publish(uint32_t v) { sensor->send(collapse::arg(v)); }
COLLAPSE_ENTRY void collapse_teardown() { logger.reset(); controller.reset(); sensor.reset(); }
