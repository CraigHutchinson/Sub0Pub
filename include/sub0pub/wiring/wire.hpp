/** Sub0Pub: Explicit wiring: Wiring and wire() (runtime addresses), StaticWiring (static storage), handles_v
 * @remark Part of Sub0Pub (https://github.com/BareCpper/Sub0Pub), MIT License: see LICENSE.md.
 *         Included by the umbrella header <sub0pub/sub0pub.hpp>.
 */
#ifndef CROG_SUB0PUB_WIRING_WIRE_HPP
#define CROG_SUB0PUB_WIRING_WIRE_HPP

#include "sub0pub/wiring/capability.hpp"
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>

//
// Receivers are ordinary classes: a non-virtual `receive(const T&)` per message type they handle, and optionally
// `bool filter(const T&)`. No base class, no table, no run-time subscription. The application binds concrete receiver
// instances where it composes itself; their types are kept all the way to the call, so each delivery is a direct,
// inlinable call (measured equal to hand-written code: docs/EVIDENCE.md):
//
//   auto bus = sub0::wire(controllerA, controllerB, logger);             // runtime addresses, static types
//   using Bus = sub0::StaticWiring<&controllerA, &controllerB, &logger>; // static storage: no RAM, fixed targets
//   struct Sensor { sub0::Sink<Sample> out; ... };                       // non-template publisher: one indirect call
//
// This is the explicit level. Reach for it when a Data type cannot decide its own delivery: receivers without static
// storage, several independent wirings of one type, a publication a receiver may stop (publishCancelable), or a
// transport's ingress (publishFrom). Otherwise say it on the type (sub0::StaticTo, sub0pub/config.hpp) and write
// publishers and receivers as for the runtime broker.
//
// Routing is by capability: publish(msg) calls, in bound order, every bound receiver that has receive(const T&).
// A receiver whose receive() returns bool stops the rest of a publishCancelable() by returning false.
// Messages never list receivers: per-message policy (sub0pub/config.hpp) and application wiring stay separate.
// Wirings add no synchronisation: concurrent publishers need stable bindings and thread-safe receivers.

namespace sub0
{
    /** Whether a wiring delivers T to R: a receiver meant to handle T can state it where it is bound, e.g.
     *      static_assert(sub0::handles_v<Logger, Sample>, "Logger must receive Sample");
     *  Capability routing is otherwise silent about a signature mismatch (docs/DESIGN.md, K14). Pass `const R` for a
     *  receiver bound through a pointer to const.
     */
    template<class R, class T>
    constexpr bool handles_v =
        detail::wiring::Accepts<std::remove_reference_t<decltype(detail::wiring::receiver(std::declval<R&>()))>, T>;

    /** Receivers bound by reference at the composition point (runtime addresses, static types)
     * @see wire()
     */
    template<class... Bound>
    class Wiring
    {
    public:
        constexpr explicit Wiring(Bound&... bound) noexcept : bound_(bound...) {}

        /// Deliver to every bound receiver that handles T, in bound order
        template<class T>
        SUB0PUB_FORCE_INLINE void publish(const T& msg) const noexcept { publish(msg, Indices{}); }

        /// As publish(), stopping at the first receiver whose bool receive() returns false
        template<class T>
        void publishCancelable(const T& msg) const noexcept { publishCancelable(msg, Indices{}); }

        /// Ingress from one of the bound receivers (e.g. a transport endpoint): every other receiver gets it
        template<class T, class Origin>
        void publishFrom(const Origin& origin, const T& msg) const noexcept { publishFrom(origin, msg, Indices{}); }

        /// Ingress identified by the origin's type, which must be bound exactly once: no origin object needed
        template<class Origin, class T>
        void publishFrom(const T& msg) const noexcept
        {
            static_assert(detail::wiring::countOf<Origin, detail::wiring::receiver_t<Bound>...> == 1,
                          "publishFrom<Origin>: Origin must be bound exactly once; pass the origin object instead");
            publishFromType<Origin>(msg, Indices{});
        }

    private:
        // Each binding is read (std::get) right before its own delivery, as hand-written code does. std::apply
        // would read every binding up front and keep them live across the calls (extra saved registers).
        using Indices = std::index_sequence_for<Bound...>;

        template<class T, std::size_t... I>
        SUB0PUB_FORCE_INLINE void publish(const T& msg, std::index_sequence<I...>) const noexcept
        {
            (detail::wiring::deliver(detail::wiring::receiver(std::get<I>(bound_)), msg), ...);
        }

        template<class T, std::size_t... I>
        void publishCancelable(const T& msg, std::index_sequence<I...>) const noexcept
        {
            (detail::wiring::deliverContinue(detail::wiring::receiver(std::get<I>(bound_)), msg) && ...);
        }

        template<class T, class Origin, std::size_t... I>
        void publishFrom(const Origin& origin, const T& msg, std::index_sequence<I...>) const noexcept
        {
            constexpr bool unique = detail::wiring::countOf<Origin, detail::wiring::receiver_t<Bound>...> == 1;
            (detail::wiring::deliverExcept<unique>(detail::wiring::receiver(std::get<I>(bound_)), msg, origin), ...);
        }

        template<class Origin, class T, std::size_t... I>
        void publishFromType(const T& msg, std::index_sequence<I...>) const noexcept
        {
            (detail::wiring::deliverExceptType<Origin>(detail::wiring::receiver(std::get<I>(bound_)), msg), ...);
        }

        // mutable: publish() is const, and a by-value adapter must stay callable through it (a const adapter
        // whose receive() is non-const would silently stop matching the capability check)
        mutable std::tuple<detail::wiring::stored_t<Bound>...> bound_;
    };

    /** Bind receivers at the composition point: `auto bus = sub0::wire(a, b, logger); bus.publish(Sample{1});` */
    template<class... Bound>
    constexpr Wiring<Bound...> wire(Bound&... bound) noexcept { return Wiring<Bound...>(bound...); }

    /** Static topology for receivers with static storage duration: the targets are template arguments, so the
     *  wiring needs no storage: `using Bus = sub0::StaticWiring<&a, &b>; Bus::publish(Sample{1});`
     */
    template<auto*... Bound>
    struct StaticWiring
    {
        /// Deliver to every bound receiver that handles T, in bound order
        template<class T>
        static SUB0PUB_FORCE_INLINE void publish(const T& msg) noexcept
        {
            (detail::wiring::deliver(detail::wiring::receiver(*Bound), msg), ...);
        }

        /// As publish(), stopping at the first receiver whose bool receive() returns false
        template<class T>
        static void publishCancelable(const T& msg) noexcept
        {
            (detail::wiring::deliverContinue(detail::wiring::receiver(*Bound), msg) && ...);
        }

        /// Ingress from one of the bound receivers: every other receiver gets it
        template<class T, class Origin>
        static void publishFrom(const Origin& origin, const T& msg) noexcept
        {
            constexpr bool unique = detail::wiring::countOf<Origin, detail::wiring::receiver_t<std::remove_pointer_t<decltype(Bound)>>...> == 1;
            (detail::wiring::deliverExcept<unique>(detail::wiring::receiver(*Bound), msg, origin), ...);
        }

        /// Ingress identified by the origin's type, which must be bound exactly once
        template<class Origin, class T>
        static void publishFrom(const T& msg) noexcept
        {
            static_assert(detail::wiring::countOf<Origin, detail::wiring::receiver_t<std::remove_pointer_t<decltype(Bound)>>...> == 1,
                          "publishFrom<Origin>: Origin must be bound exactly once; pass the origin object instead");
            (detail::wiring::deliverExceptType<Origin>(detail::wiring::receiver(*Bound), msg), ...);
        }
    };
} // END: sub0

#endif // CROG_SUB0PUB_WIRING_WIRE_HPP
