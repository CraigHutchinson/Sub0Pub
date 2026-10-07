/** Case: default and runtime filters: Controller declares an always-true filter (must compile away); EvenMonitor's runtime filter keeps its branch.
 *  The typed topology: the message type names its receivers (sub0::StaticTo) and opts in to filter() (sub0::Filter),
 *  as it must on the runtime broker; receivers and publisher are written as for the broker. */
#include "collapse_case.hpp"
#include "sub0pub/sub0pub.hpp"

namespace {
// A receiver is named before it is defined, as an application declares `extern Controller controller;`. The
// harness constructs its objects in collapse_setup(), through a holder with the storage of collapse::Slot.
struct ControllerSlot;
struct MonitorSlot;
extern ControllerSlot controller;
extern MonitorSlot monitor;
struct Sample { uint32_t value; using sub0_config = sub0::config<sub0::Filter, sub0::StaticTo<&controller, &monitor>>; };
struct Controller final : sub0::Subscribe<Sample> {
    bool filter(const Sample&) noexcept { return true; }
    void receive(const Sample& s) noexcept { COLLAPSE_WORK(s.value * 3U); }
};
struct EvenMonitor final : sub0::Subscribe<Sample> {
    bool filter(const Sample& s) noexcept { return (s.value & 1U) == 0U; }
    void receive(const Sample& s) noexcept { COLLAPSE_WORK(s.value + 7U); }
};
struct ControllerSlot : collapse::Slot<Controller> {};
struct MonitorSlot : collapse::Slot<EvenMonitor> {};
struct Sensor : sub0::Publish<Sample> {
    void send(uint32_t v) noexcept { sub0::publish(*this, Sample{v}); }
};
ControllerSlot controller;
MonitorSlot monitor;
collapse::Slot<Sensor> sensor;
}

COLLAPSE_ENTRY void collapse_setup() { controller.emplace(); monitor.emplace(); sensor.emplace(); }
COLLAPSE_ENTRY void collapse_publish(uint32_t v) { sensor->send(collapse::arg(v)); }
COLLAPSE_ENTRY void collapse_teardown() { sensor.reset(); monitor.reset(); controller.reset(); }
