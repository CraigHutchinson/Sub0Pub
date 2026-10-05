/** Sub0Pub: Transport endpoints in a wiring: Forward<Transport> and StaticForward<&transport>
 * @remark Part of Sub0Pub (https://github.com/BareCpper/Sub0Pub), MIT License: see LICENSE.md.
 *         Included by the umbrella header <sub0pub/sub0pub.hpp>.
 */
#ifndef CROG_SUB0PUB_WIRING_FORWARD_HPP
#define CROG_SUB0PUB_WIRING_FORWARD_HPP

#include "sub0pub/wiring/capability.hpp"
#include <type_traits>
#include <utility>

namespace sub0
{
    /** Transport endpoint binding: forwards every message type the transport can send.
     *  Transport concept: send(const T&) for each message type it carries (result handling is the transport's).
     *  Ingress from the transport: wiring.publishFrom(transport, msg) skips this binding (split horizon).
     */
    template<class Transport>
    class Forward
    {
    public:
        using sub0_by_value = void;       // refers to the transport only: a Wiring holds it by value (one hop)
        using sub0_endpoint = Transport;  // split horizon: ingress may name this adapter or the transport itself

        constexpr explicit Forward(Transport& transport) noexcept : transport_(transport) {}

        /// Split-horizon identity: the transport it forwards to
        constexpr const void* sub0_identity() const noexcept { return &transport_; }

        template<class T>
        auto receive(const T& msg) const noexcept -> decltype(std::declval<Transport&>().send(msg), void())
        {
            transport_.send(msg);
        }

    private:
        Transport& transport_;
    };

    /** Transport endpoint binding for a transport with static storage: no RAM, fixed target */
    template<auto* TransportObject>
    struct StaticForward
    {
        /// Split horizon: ingress may name this binding or the transport itself as its origin
        using sub0_endpoint = detail::wiring::receiver_t<std::remove_pointer_t<decltype(TransportObject)>>;
        constexpr const void* sub0_identity() const noexcept { return &detail::wiring::receiver(*TransportObject); }

        template<class T>
        auto receive(const T& msg) noexcept -> decltype(detail::wiring::receiver(*TransportObject).send(msg), void())
        {
            detail::wiring::receiver(*TransportObject).send(msg);
        }
    };
} // END: sub0

#endif // CROG_SUB0PUB_WIRING_FORWARD_HPP
