/** Sub0Pub: Wiring internals: capability detection and delivery to bound receivers
 * @remark Part of Sub0Pub (https://github.com/BareCpper/Sub0Pub), MIT License: see LICENSE.md.
 *         Included by the umbrella header <sub0pub/sub0pub.hpp>.
 */
#ifndef CROG_SUB0PUB_WIRING_CAPABILITY_HPP
#define CROG_SUB0PUB_WIRING_CAPABILITY_HPP

#include "sub0pub/config_macros.hpp"
#include <cassert>
#include <concepts>
#include <cstddef>
#include <type_traits>
#include <utility>

namespace sub0
{
    namespace detail
    {
    namespace wiring
    {
        template<class R, class T>
        concept Accepts = requires(R& receiver, const T& message) { receiver.receive(message); };

        // Preserve explicit boolean conversion (not just implicit convertibility) for filter results.
        template<class R, class T>
        concept HasFilter = requires(R& receiver, const T& message) { bool(receiver.filter(message)); };

        /// Bound objects may be the receiver itself or a holder exposing it via get() (e.g. application storage slots)
        template<class X>
        concept HasGet = requires(X& value) { value.get(); };

        template<class X>
        constexpr decltype(auto) receiver(X& x) noexcept
        {
            if constexpr (HasGet<X>)
                return x.get();
            else
                return (x);
        }

        template<class R, class T>
        concept ReturnsBool = requires(R& receiver, const T& message)
        {
            { receiver.receive(message) } -> std::same_as<bool>;
        };

        /// Deliver to one receiver: nothing at all if it does not handle T; its filter only if it declares one
        template<class R, class T>
        SUB0PUB_FORCE_INLINE void deliver(R& r, const T& msg) noexcept
        {
            if constexpr (Accepts<R, T>)
            {
                if constexpr (HasFilter<R, T>)
                    if (!r.filter(msg))
                        return;
                r.receive(msg);
            }
        }

        /// Deliver to one receiver; returns whether the publication continues. A receiver that does not handle T,
        /// is filtered out, or returns void always continues; one returning bool stops it with false.
        template<class R, class T>
        inline bool deliverContinue(R& r, const T& msg) noexcept
        {
            if constexpr (Accepts<R, T>)
            {
                if constexpr (HasFilter<R, T>)
                    if (!r.filter(msg))
                        return true;
                if constexpr (ReturnsBool<R, T>)
                    return r.receive(msg);
                else
                {
                    r.receive(msg);
                    return true;
                }
            }
            else
                return true;
        }

        /// A binding adapter that only refers to the real endpoint (e.g. Forward<Transport>) declares
        /// `using sub0_by_value = void;` so a Wiring holds it by value: one hop to the endpoint, as hand-written
        template<class B, class = void> struct by_value : std::false_type {};
        template<class B> struct by_value<B, std::void_t<typename B::sub0_by_value>> : std::true_type {};
        template<class B> using stored_t = std::conditional_t<by_value<B>::value, B, B&>;

        /// Endpoint identity for split horizon: the bound object, or for an adapter what it refers to
        template<class X, class = void> struct has_identity : std::false_type {};
        template<class X> struct has_identity<X, std::void_t<decltype(std::declval<const X&>().sub0_identity())>> : std::true_type {};
        template<class X>
        constexpr const void* identity(const X& x) noexcept
        {
            if constexpr (has_identity<X>::value)
                return x.sub0_identity();
            else
                return static_cast<const void*>(&x);
        }

        template<class X> using receiver_t = std::remove_cv_t<std::remove_reference_t<decltype(receiver(std::declval<X&>()))>>;

        /// The endpoint a binding stands for: an adapter that forwards to a transport declares
        /// `using sub0_endpoint = Transport;`, so ingress may name either the adapter or the transport as its origin
        template<class X, class = void> struct endpoint_of { using type = X; };
        template<class X> struct endpoint_of<X, std::void_t<typename X::sub0_endpoint>> { using type = typename X::sub0_endpoint; };

        /// Whether a binding of type R is the endpoint an ingress origin of type Origin refers to
        template<class R, class Origin>
        constexpr bool isOrigin = std::is_same_v<std::remove_cv_t<R>, std::remove_cv_t<Origin>> ||
                                  std::is_same_v<typename endpoint_of<std::remove_cv_t<R>>::type, std::remove_cv_t<Origin>>;

        template<class Origin, class... R>
        constexpr std::size_t countOf = (std::size_t(isOrigin<R, Origin>) + ... + std::size_t(0));

        /// Split horizon: do not send a message back to the binding it came from. When the origin's type is bound
        /// exactly once (OriginUnique), the origin *is* that binding: decided at compile time, no address compare
        /// (precondition: the origin is one of the bound endpoints, asserted in debug builds).
        template<bool OriginUnique, class R, class T, class Origin>
        inline void deliverExcept(R& r, const T& msg, const Origin& origin) noexcept
        {
            // No statement follows a skip, so no instantiation holds code it cannot reach (MSVC C4702)
            if constexpr (isOrigin<R, Origin>)
            {
                if constexpr (OriginUnique)
                {
                    assert(identity(r) == identity(origin) && "publishFrom: origin is not a bound endpoint");
                    (void)r; (void)msg; (void)origin;
                }
                else if (identity(r) != identity(origin))
                    deliver(r, msg);
            }
            else
                deliver(r, msg);
        }

        /// Origin identified by type alone (publishFrom<Origin>(msg)): the one binding of that type is skipped
        template<class Origin, class R, class T>
        inline void deliverExceptType(R& r, const T& msg) noexcept
        {
            if constexpr (!isOrigin<R, Origin>)
                deliver(r, msg);
        }
    } // END: wiring
    } // END: detail
} // END: sub0

#endif // CROG_SUB0PUB_WIRING_CAPABILITY_HPP
