#pragma once
/** Case: cross-file application (receivers in another translation unit, external linkage).
 *  Pattern B1: typed wiring bound at the composition point, receivers implemented in another TU. Built with and without LTO. */
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
} // namespace app
