#pragma once
/** Case: cross-file application (receivers in another translation unit, external linkage).
 *  Reference: direct calls to receivers implemented in another TU. Built with and without LTO. */
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
} // namespace app
