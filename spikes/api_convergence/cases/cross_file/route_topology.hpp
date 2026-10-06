#pragma once
/** Case cross_file, candidate "typed route": the header every translation unit of the application includes.
 *  It plays the part of a project topology header: it names the receivers before they are defined, states the
 *  message type's topology (ROUTE_TOPOLOGY, set by the including variant), then defines the receivers, whose
 *  receive() bodies live in another translation unit. Built with and without link-time optimisation. */
#include "collapse_case.hpp"
#include "sub0pub_spike/topology.hpp"

#define ROUTE_DYNAMIC 0
#define ROUTE_STATIC 1

namespace app {
struct Sample { uint32_t value; };
// A real application declares `extern Controller controllerA;` here. The harness constructs its objects in
// collapse_setup(), through holders; a holder is named before its receiver is complete, as the object would be.
struct ControllerSlot;
struct LoggerSlot;
extern ControllerSlot controllerA;
extern ControllerSlot controllerB;
extern LoggerSlot logger;
} // namespace app

#if ROUTE_TOPOLOGY == ROUTE_STATIC
SUB0PUB_CONFIGURE(app::Sample, sub0::spike::StaticTo<&app::controllerA, &app::controllerB, &app::logger>);
#endif

namespace app {
struct Controller final : sub0::spike::Subscribe<Sample> {
    explicit Controller(uint32_t g) noexcept : gain(g) {}
    void receive(const Sample& s) noexcept;   // defined in receivers.cpp
    uint32_t gain;
};

struct Logger final : sub0::spike::Subscribe<Sample> {
    void receive(const Sample& s) noexcept;   // defined in receivers.cpp
    uint32_t count = 0;
};

struct ControllerSlot : collapse::Slot<Controller> {};
struct LoggerSlot : collapse::Slot<Logger> {};
} // namespace app
