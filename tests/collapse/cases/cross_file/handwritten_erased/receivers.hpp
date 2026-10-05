#pragma once
/** Case: cross-file application (receivers in another translation unit, external linkage).
 *  Type-erased reference: a non-template publisher in its own TU (sensor.cpp, a library boundary) holding a
 *  context + function pointer; the application's node (app.cpp) delivers to the receivers (receivers.cpp).
 *  Equal work for B3, selected with `// COLLAPSE_REFERENCE: handwritten_erased`. Built with and without LTO. */
#include "collapse_case.hpp"

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
    const void* ctx;
    void (*fn)(const void*, const Sample&) noexcept;
    void send(uint32_t v) noexcept;           // defined in sensor.cpp
};
} // namespace app
