#include "receivers.hpp"

namespace app {
void Sensor::send(uint32_t v) noexcept { fn(ctx, Sample{v}); }
}
