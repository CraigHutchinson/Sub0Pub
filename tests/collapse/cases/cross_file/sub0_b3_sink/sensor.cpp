#include "receivers.hpp"

namespace app {
void Sensor::send(uint32_t v) noexcept { out.publish(Sample{v}); }
}
