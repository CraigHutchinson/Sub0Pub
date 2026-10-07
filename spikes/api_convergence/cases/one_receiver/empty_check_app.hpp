#pragma once
/** Case one_receiver: the price of reporting a publication that reaches nobody.
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
struct Controller final : sub0::Subscribe<Sample> {
    explicit Controller(uint32_t g) noexcept : gain(g) {}
    void receive(const Sample& s) noexcept override { COLLAPSE_WORK(s.value * gain); }
    uint32_t gain;
};
struct Sensor : sub0::Publish<Sample> {
    void send(uint32_t v) noexcept { sub0::publish(*this, Sample{v}); }
};
collapse::Slot<Sensor> sensor;
collapse::Slot<Controller> controller;
}

COLLAPSE_ENTRY void collapse_setup() { sensor.emplace(); controller.emplace(3U); }
COLLAPSE_ENTRY void collapse_publish(uint32_t v) { sensor->send(collapse::arg(v)); }
COLLAPSE_ENTRY void collapse_teardown() { controller.reset(); sensor.reset(); }
