#pragma once
/** Case: cross-file application (receivers in another translation unit, external linkage).
 *  Pattern B3: a non-template publisher in its own TU (sensor.cpp, a library boundary) holding a Sink<Sample>;
 *  the wiring is built in app.cpp, the receivers are in receivers.cpp. Built with and without LTO. */
#include "collapse_case.hpp"
#include "sub0pub/sub0pub.hpp"

namespace app {
struct Sample { uint32_t value; };

struct Controller {
    explicit Controller(uint32_t g) noexcept : gain(g) {}
    void receive(const Sample& s) noexcept;   // defined in receivers.cpp
    uint32_t gain;
};

struct Logger {
    void receive(const Sample& s) noexcept;   // defined in receivers.cpp
    uint32_t count = 0;
};

/// The library's publisher: knows only the message type
struct Sensor {
    explicit Sensor(sub0::Sink<Sample> o) noexcept : out(o) {}
    void send(uint32_t v) noexcept;           // defined in sensor.cpp
    sub0::Sink<Sample> out;
};
} // namespace app
