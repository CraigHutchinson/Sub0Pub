/** Case cross_file. Candidate "typed route", StaticTo an out-of-line route: the publisher's translation unit sees
 *  only the route's declaration, not the receivers. Judged against hand-written direct calls to the same
 *  out-of-line receivers: it prices decoupling publishers from receivers, with and without LTO. */
#include "../route_out_of_line.hpp"

namespace app {
ControllerSlot controllerA;
ControllerSlot controllerB;
LoggerSlot logger;
}

namespace {
struct Sensor : sub0::spike::Publish<app::Sample> {
    void send(uint32_t v) noexcept { sub0::spike::publish(*this, app::Sample{v}); }
};
collapse::Slot<Sensor> sensor;
}

COLLAPSE_ENTRY void collapse_setup() { sensor.emplace(); app::controllerA.emplace(3U); app::controllerB.emplace(5U); app::logger.emplace(); }
COLLAPSE_ENTRY void collapse_publish(uint32_t v) { sensor->send(collapse::arg(v)); }
COLLAPSE_ENTRY void collapse_teardown() { app::logger.reset(); app::controllerB.reset(); app::controllerA.reset(); sensor.reset(); }
