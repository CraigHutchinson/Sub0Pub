/** Case: 32 receivers of one type.
 *  The typed topology: the message type names its receivers (sub0::StaticTo); receivers and publisher are written
 *  as for the runtime broker (public API, include/sub0pub/sub0pub.hpp). */
#include "collapse_case.hpp"
#include "sub0pub/sub0pub.hpp"
#include "many.hpp"

namespace {
// A receiver is named before it is defined, as an application declares `extern Controller controller;`. The
// harness constructs its objects in collapse_setup(), through a holder with the storage of collapse::Slot.
struct ControllerSlot;
#define MANY_EXTERN(i) extern ControllerSlot c##i;
MANY_EACH(MANY_EXTERN)
struct Sample { uint32_t value; using sub0_config = sub0::config<sub0::StaticTo<MANY_ADDRESSES>>; };
struct Controller final : sub0::Subscribe<Sample> {
    explicit Controller(uint32_t g) noexcept : gain(g) {}
    void receive(const Sample& s) noexcept { COLLAPSE_WORK(s.value * gain); }
    uint32_t gain;
};
struct ControllerSlot : collapse::Slot<Controller> {};
#define MANY_DECLARE(i) ControllerSlot c##i;
MANY_EACH(MANY_DECLARE)
struct Sensor : sub0::Publish<Sample> {
    void send(uint32_t v) noexcept { sub0::publish(*this, Sample{v}); }
};
collapse::Slot<Sensor> sensor;
}

#define MANY_EMPLACE(i) c##i.emplace(i + 1U);
#define MANY_RESET(i) c##i.reset();
COLLAPSE_ENTRY void collapse_setup() { MANY_EACH(MANY_EMPLACE) sensor.emplace(); }
COLLAPSE_ENTRY void collapse_publish(uint32_t v) { sensor->send(collapse::arg(v)); }
COLLAPSE_ENTRY void collapse_teardown() { sensor.reset(); MANY_EACH(MANY_RESET) }
