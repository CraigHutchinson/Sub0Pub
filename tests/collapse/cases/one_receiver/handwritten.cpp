/** Case: one concrete receiver. Reference: a direct call to the receiver object. */
#include "collapse_case.hpp"

namespace {
struct Sample { uint32_t value; };
struct Controller {
    explicit Controller(uint32_t g) noexcept : gain(g) {}
    void receive(const Sample& s) noexcept { COLLAPSE_WORK(s.value * gain); }
    uint32_t gain;
};
collapse::Slot<Controller> controller;
}

COLLAPSE_ENTRY void collapse_setup() { controller.emplace(3U); }
COLLAPSE_ENTRY void collapse_publish(uint32_t v) { controller->receive(Sample{collapse::arg(v)}); }
COLLAPSE_ENTRY void collapse_teardown() { controller.reset(); }
