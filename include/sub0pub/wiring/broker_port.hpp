/** Sub0Pub: BrokerPort<T>: runtime subscribers behind a static wiring, through the runtime broker
 * @remark Part of Sub0Pub (https://github.com/BareCpper/Sub0Pub), MIT License: see LICENSE.md.
 *         Included by the umbrella header <sub0pub/sub0pub.hpp>.
 */
#ifndef CROG_SUB0PUB_WIRING_BROKER_PORT_HPP
#define CROG_SUB0PUB_WIRING_BROKER_PORT_HPP

#include "sub0pub/broker/publish.hpp"
#include <type_traits>

namespace sub0
{
    /** Runtime subscribers behind an explicit wiring, with the full per-type broker (sub0pub/config.hpp policy) on the
     *  dynamic side: bind the port like any receiver; Subscribe<T> objects receive through it.
     * @remark For a Data type whose known receivers have static storage, sub0::StaticFirst says the same thing on the type.
     */
    template<class T>
    class BrokerPort : public Publish<T>
    {
    public:
        template<class C = config_t<T>, std::enable_if_t<C::storage == Storage::Global, int> = 0>
        BrokerPort() noexcept {}

        template<class C = config_t<T>, std::enable_if_t<C::storage == Storage::Scoped, int> = 0>
        explicit BrokerPort(Domain<T>& domain) noexcept : Publish<T>(domain) {}

        void receive(const T& msg) noexcept { this->publish(msg); }
    };
} // END: sub0

#endif // CROG_SUB0PUB_WIRING_BROKER_PORT_HPP
