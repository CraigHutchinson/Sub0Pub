#pragma once
/** Case: cross-file application (receivers in another translation unit, external linkage).
 *  The runtime broker (Subscribe/Publish), virtual receivers implemented in another TU. Built with and without LTO. */
// Today's API at its leanest settings (fair comparison): direct dispatch, no assertion checks
#define SUB0PUB_ASSERT false
#include "collapse_case.hpp"
#include "sub0pub/sub0pub.hpp"

namespace app {
struct Sample { uint32_t value; };

struct Controller final : sub0::Subscribe<Sample> {
    explicit Controller(uint32_t g) noexcept : gain(g) {}
    void receive(const Sample& s) noexcept override;   // defined in receivers.cpp
    uint32_t gain;
};

struct Logger final : sub0::Subscribe<Sample> {
    void receive(const Sample& s) noexcept override;   // defined in receivers.cpp
    uint32_t count = 0;
};
} // namespace app
