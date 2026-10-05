/** Case: 32 receivers of one type. The runtime broker (Subscribe/Publish) at its leanest macros,
 *  with a 32-entry subscription table. */
// Today's API at its leanest settings (fair comparison): direct dispatch, no assertion checks
#define SUB0PUB_REENTRANT_SAFE false
#define SUB0PUB_ASSERT false
#define SUB0PUB_MAX_SUBSCRIPTIONS 32
#include "collapse_case.hpp"
#include "sub0pub/sub0pub.hpp"
#include "many.hpp"

namespace {
struct Sample { uint32_t value; };
struct Controller final : sub0::Subscribe<Sample> {
    explicit Controller(uint32_t g) noexcept : gain(g) {}
    void receive(const Sample& s) noexcept override { COLLAPSE_WORK(s.value * gain); }
    uint32_t gain;
};
struct Sensor : sub0::Publish<Sample> {
    void send(uint32_t v) noexcept { sub0::publish(*this, Sample{v}); }
};
collapse::Slot<Sensor> sensor;
#define MANY_DECLARE(i) collapse::Slot<Controller> c##i;
MANY_EACH(MANY_DECLARE)
}

#define MANY_EMPLACE(i) c##i.emplace(i + 1U);
#define MANY_RESET(i) c##i.reset();
COLLAPSE_ENTRY void collapse_setup() { sensor.emplace(); MANY_EACH(MANY_EMPLACE) }
COLLAPSE_ENTRY void collapse_publish(uint32_t v) { sensor->send(collapse::arg(v)); }
COLLAPSE_ENTRY void collapse_teardown() { MANY_EACH(MANY_RESET) sensor.reset(); }
