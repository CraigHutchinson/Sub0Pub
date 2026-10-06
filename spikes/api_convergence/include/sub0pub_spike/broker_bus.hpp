#pragma once

/** @file broker_bus.hpp
 *  Spike "bus parameter": the runtime broker presented as a wiring, so a publisher written against an output
 *  type (`Thermometer<Bus>`) takes the broker, a StaticWiring or a wire() result without change.
 */

#include "sub0pub/wiring/broker_port.hpp"

namespace sub0::spike
{
    /** A wiring whose only target is the runtime broker: publish(msg) reaches every Subscribe<T> of msg's type.
     * @note Global storage only; a Scoped type needs a BrokerPort bound to its Domain.
     */
    struct BrokerBus
    {
        /** Delivers msg through the runtime broker for its type.
         * @param msg  The message to publish.
         */
        template<class T>
        static void publish(const T& msg) noexcept
        {
            BrokerPort<T> port; // an empty handle for Global storage
            port.receive(msg);
        }
    };
} // namespace sub0::spike
