/** Per-Data configuration alone: resolving a type's policy needs no broker, wiring or IPC */
#include "sub0pub/config.hpp"

#if defined(CROG_SUB0PUB_BROKER_HPP) || defined(CROG_SUB0PUB_WIRING_HPP) || defined(CROG_SUB0PUB_IPC_HPP)
#error "sub0pub/config.hpp must not include the broker, static wiring or IPC"
#endif

namespace {
struct ConfiguredMsg { using sub0_config = sub0::config<sub0::Capacity<4>>; };
}

int useConfig()
{
    return static_cast<int>(sub0::config_t<ConfiguredMsg>::capacity);
}
