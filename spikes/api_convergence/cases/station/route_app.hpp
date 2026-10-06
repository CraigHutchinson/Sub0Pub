#pragma once
/** Case station (two message types, three receivers), candidate "typed route".
 *  The participants below are written once, in the runtime broker's spelling. The including variant selects the
 *  topology (ROUTE_TOPOLOGY); the per-type configuration block is the only thing that differs between builds. */
#include "collapse_case.hpp"
#include "sub0pub_spike/topology.hpp"

#define ROUTE_DYNAMIC 0
#define ROUTE_STATIC 1
#define ROUTE_BRIDGED 2

#ifndef ROUTE_OVERRIDE
#define ROUTE_OVERRIDE // set to override only by the zero-edit variant (SUB0PUB_SPIKE_STATIC_VIRTUAL)
#endif
#ifndef ROUTE_LAYOUT
#define ROUTE_LAYOUT // set to __declspec(empty_bases) only by the variant that prices the MSVC empty-base layout
#endif

namespace {
struct Sample { uint32_t value; };
struct Command { uint32_t code; };
// A real application declares `extern Controller controller;` here. The harness constructs its objects in
// collapse_setup(), through holders; a holder is named before its receiver is complete, as the object would be.
struct ControllerSlot;
struct LoggerSlot;
struct ActuatorSlot;
extern ControllerSlot controller;
extern LoggerSlot logger;
extern ActuatorSlot actuator;
}

#if ROUTE_TOPOLOGY == ROUTE_STATIC
SUB0PUB_CONFIGURE(Sample, sub0::spike::StaticTo<&controller, &logger>);
SUB0PUB_CONFIGURE(Command, sub0::spike::StaticTo<&controller, &actuator>);
#elif ROUTE_TOPOLOGY == ROUTE_BRIDGED
SUB0PUB_CONFIGURE(Sample, sub0::spike::StaticFirst<&controller, &logger>);
SUB0PUB_CONFIGURE(Command, sub0::spike::StaticFirst<&controller, &actuator>);
#endif

namespace {
namespace s0 = sub0::spike;
struct ROUTE_LAYOUT Controller final : s0::Subscribe<Sample>, s0::Subscribe<Command> {
    void receive(const Sample& s) noexcept ROUTE_OVERRIDE { COLLAPSE_WORK(s.value * 3U); }
    void receive(const Command& c) noexcept ROUTE_OVERRIDE { COLLAPSE_WORK(c.code + 1U); }
};
struct Logger final : s0::Subscribe<Sample> {
    void receive(const Sample& s) noexcept ROUTE_OVERRIDE { ++count; COLLAPSE_WORK(s.value ^ count); }
    uint32_t count = 0;
};
struct Actuator final : s0::Subscribe<Command> {
    void receive(const Command& c) noexcept ROUTE_OVERRIDE { COLLAPSE_WORK(c.code << 2U); }
};
struct ROUTE_LAYOUT Sensor : s0::Publish<Sample>, s0::Publish<Command> {
    void send(uint32_t v) noexcept
    {
        s0::publish(*this, Sample{v});
        s0::publish(*this, Command{v ^ 0xFU});
    }
};
struct ControllerSlot : collapse::Slot<Controller> {};
struct LoggerSlot : collapse::Slot<Logger> {};
struct ActuatorSlot : collapse::Slot<Actuator> {};
collapse::Slot<Sensor> sensor;
ControllerSlot controller;
LoggerSlot logger;
ActuatorSlot actuator;
}

COLLAPSE_ENTRY void collapse_setup() { sensor.emplace(); controller.emplace(); logger.emplace(); actuator.emplace(); }
COLLAPSE_ENTRY void collapse_publish(uint32_t v) { sensor->send(collapse::arg(v)); }
COLLAPSE_ENTRY void collapse_teardown() { actuator.reset(); logger.reset(); controller.reset(); sensor.reset(); }
