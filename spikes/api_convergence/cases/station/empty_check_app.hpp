#pragma once
/** Case station: the price of reporting a publication that reaches nobody.
 *  Today's Subscribe / Publish code over the spike's stand-in broker (sub0pub_spike/no_receivers.hpp), selected next
 *  to the message type. The including variant chooses the broker with or without the check (EMPTY_CHECK_OPTION);
 *  both are judged against the library broker, so the difference between them is the check. */
#include "collapse_case.hpp"
#include "sub0pub_spike/no_receivers.hpp"

namespace {
struct Sample {
    uint32_t value;
    using sub0_config = sub0::config<EMPTY_CHECK_OPTION>;
};
struct Command {
    uint32_t code;
    using sub0_config = sub0::config<EMPTY_CHECK_OPTION>;
};
struct Controller final : sub0::Subscribe<Sample>, sub0::Subscribe<Command> {
    void receive(const Sample& s) noexcept override { COLLAPSE_WORK(s.value * 3U); }
    void receive(const Command& c) noexcept override { COLLAPSE_WORK(c.code + 1U); }
};
struct Logger final : sub0::Subscribe<Sample> {
    void receive(const Sample& s) noexcept override { ++count; COLLAPSE_WORK(s.value ^ count); }
    uint32_t count = 0;
};
struct Actuator final : sub0::Subscribe<Command> {
    void receive(const Command& c) noexcept override { COLLAPSE_WORK(c.code << 2U); }
};
struct Sensor : sub0::Publish<Sample>, sub0::Publish<Command> {
    void send(uint32_t v) noexcept
    {
        sub0::publish(*this, Sample{v});
        sub0::publish(*this, Command{v ^ 0xFU});
    }
};
collapse::Slot<Sensor> sensor;
collapse::Slot<Controller> controller;
collapse::Slot<Logger> logger;
collapse::Slot<Actuator> actuator;
}

COLLAPSE_ENTRY void collapse_setup() { sensor.emplace(); controller.emplace(); logger.emplace(); actuator.emplace(); }
COLLAPSE_ENTRY void collapse_publish(uint32_t v) { sensor->send(collapse::arg(v)); }
COLLAPSE_ENTRY void collapse_teardown() { actuator.reset(); logger.reset(); controller.reset(); sensor.reset(); }
