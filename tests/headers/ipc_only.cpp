/** IPC serialisation alone: no runtime broker, configuration or static wiring is reachable from sub0pub/ipc.hpp */
#include "sub0pub/ipc.hpp"

#if defined(CROG_SUB0PUB_CONFIG_HPP) || defined(CROG_SUB0PUB_BROKER_HPP) || defined(CROG_SUB0PUB_BROKER_TABLE_HPP) \
    || defined(CROG_SUB0PUB_BROKER_SUBSCRIBE_HPP) || defined(CROG_SUB0PUB_BROKER_PUBLISH_HPP)
#error "sub0pub/ipc.hpp must not include the runtime broker or its configuration"
#endif
#if defined(CROG_SUB0PUB_WIRING_HPP) || defined(CROG_SUB0PUB_WIRING_CAPABILITY_HPP)
#error "sub0pub/ipc.hpp must not include static wiring"
#endif

int useIpc()
{
    return sizeof(sub0::DefaultSerialisation::Header) > 0 ? 1 : 0;
}
