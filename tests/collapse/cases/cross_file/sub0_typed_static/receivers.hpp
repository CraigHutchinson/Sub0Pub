#pragma once
/** Case: cross-file application (receivers in another translation unit, external linkage).
 *  The typed topology: the message type names its receivers (sub0::StaticTo) in the header both units include;
 *  receivers implemented in another TU. Built with and without LTO. */
#include "collapse_case.hpp"
#include "sub0pub/sub0pub.hpp"

namespace app {
// A receiver is named before it is defined, as an application declares `extern Controller controllerA;`. The
// harness constructs its objects in collapse_setup(), through a holder with the storage of collapse::Slot.
struct ControllerSlot;
struct LoggerSlot;
extern ControllerSlot controllerA;
extern ControllerSlot controllerB;
extern LoggerSlot logger;

struct Sample { uint32_t value; using sub0_config = sub0::config<sub0::StaticTo<&controllerA, &controllerB, &logger>>; };

struct Controller final : sub0::Subscribe<Sample> {
    explicit Controller(uint32_t g) noexcept : gain(g) {}
    void receive(const Sample& s) noexcept;   // defined in receivers.cpp
    uint32_t gain;
};

struct Logger final : sub0::Subscribe<Sample> {
    void receive(const Sample& s) noexcept;   // defined in receivers.cpp
    uint32_t count = 0;
};

struct ControllerSlot : collapse::Slot<Controller> {};
struct LoggerSlot : collapse::Slot<Logger> {};
} // namespace app
