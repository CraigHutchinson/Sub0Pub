#pragma once
/** Case one_receiver, candidate "typed route".
 *  The participants below are written once, in the runtime broker's spelling. The including variant selects the
 *  topology (ROUTE_TOPOLOGY); the per-type configuration line is the only thing that differs between builds. */
#include "collapse_case.hpp"
#include "sub0pub_spike/topology.hpp"

#define ROUTE_DYNAMIC 0
#define ROUTE_STATIC 1
#define ROUTE_BRIDGED 2

#ifndef ROUTE_OVERRIDE
#define ROUTE_OVERRIDE // set to override only by the zero-edit variant (SUB0PUB_SPIKE_STATIC_VIRTUAL)
#endif

namespace {
struct Sample { uint32_t value; };
// A real application declares `extern Controller controller;` here. The harness constructs its objects in
// collapse_setup(), through a holder; the holder is named before its receiver is complete, as the object would be.
struct ControllerSlot;
extern ControllerSlot controller;
}

#if ROUTE_TOPOLOGY == ROUTE_STATIC
SUB0PUB_CONFIGURE(Sample, sub0::spike::StaticTo<&controller>);
#elif ROUTE_TOPOLOGY == ROUTE_BRIDGED
SUB0PUB_CONFIGURE(Sample, sub0::spike::StaticFirst<&controller>);
#endif

namespace {
namespace s0 = sub0::spike;
struct Controller final : s0::Subscribe<Sample> {
    explicit Controller(uint32_t g) noexcept : gain(g) {}
    void receive(const Sample& s) noexcept ROUTE_OVERRIDE { COLLAPSE_WORK(s.value * gain); }
    uint32_t gain;
};
struct Sensor : s0::Publish<Sample> {
    void send(uint32_t v) noexcept { s0::publish(*this, Sample{v}); }
};
struct ControllerSlot : collapse::Slot<Controller> {};
collapse::Slot<Sensor> sensor;
ControllerSlot controller;
}

COLLAPSE_ENTRY void collapse_setup() { sensor.emplace(); controller.emplace(3U); }
COLLAPSE_ENTRY void collapse_publish(uint32_t v) { sensor->send(collapse::arg(v)); }
COLLAPSE_ENTRY void collapse_teardown() { controller.reset(); sensor.reset(); }
