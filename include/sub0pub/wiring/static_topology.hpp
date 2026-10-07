/** Sub0Pub: StaticTo / StaticFirst: a Data type's configuration wired to fixed receivers
 * @remark Part of Sub0Pub (https://github.com/BareCpper/Sub0Pub), MIT License: see LICENSE.md.
 *         Included by the umbrella header <sub0pub/sub0pub.hpp>.
 */
#ifndef CROG_SUB0PUB_WIRING_STATIC_TOPOLOGY_HPP
#define CROG_SUB0PUB_WIRING_STATIC_TOPOLOGY_HPP

#include "sub0pub/audit.hpp"
#include "sub0pub/config.hpp"
#include "sub0pub/wiring/capability.hpp"
#include "sub0pub/wiring/wire.hpp"
#include <cstdint>
#include <type_traits>

//
// A Data type says, next to its own definition, which receivers it is delivered to:
//
//   class Display;
//   extern Display display;
//   struct Reading { int celsius; using sub0_config = sub0::config<sub0::StaticTo<&display>>; };
//
// Subscribe<Reading>, Publish<Reading> and sub0::publish() then compile to a direct call of display.receive(), from
// the same source that would use the runtime broker without the option. This header is the bridge between the two
// areas: the options are declared with the rest of the configuration vocabulary (sub0pub/config.hpp), the broker's
// classes follow whatever topology a type's configuration names, and the delivery itself is StaticWiring.
//
// A promotion cannot fail silently. A receiver the list names that subscribes to the type must accept it; a filter()
// needs the type's sub0::Filter and the signature the broker calls; a list nobody in it can receive from is rejected
// unless the type says sub0::AllowNoReceivers; and a publisher whose translation unit does not see a receiver's
// definition does not compile.

namespace sub0
{
    namespace detail
    {
        /// Whether `subscriber` is the Base subobject of the receiver a binding stands for
        template<class Base, class Binding>
        bool isBound(Binding* binding, const Base* subscriber) noexcept
        {
            if constexpr (std::is_base_of_v<Base, wiring::receiver_t<Binding>>)
                return static_cast<const Base*>(&wiring::receiver(*binding)) == subscriber;
            else
                return false;
        }

        /// Whether R's filter() for Data is the one the runtime broker calls: `bool filter(const Data&) noexcept`
        template<class R, class Data>
        concept BrokerFilter = requires { static_cast<bool (R::*)(const Data&) noexcept>(&R::filter); };

        template<bool Open, auto*... Bound>
        struct StaticTopology : StaticWiring<Bound...>
        {
            /// The number of bound receivers that receive Data: a constant of the program
            template<class Data>
            static constexpr uint32_t cReceivers =
                (uint32_t(handles_v<std::remove_pointer_t<decltype(Bound)>, Data>) + ... + uint32_t(0));

            /** Whether a subscriber belongs to one of the bound receivers
             * @param[in] subscriber  The Subscribe<Data> base of a receiver
             */
            template<class Base>
            static bool binds(const Base* subscriber) noexcept
            {
                return (isBound(Bound, subscriber) || ...);
            }

            /// Deliver to every bound receiver that handles Data, in bound order
            template<class Data>
            static SUB0PUB_FORCE_INLINE void publish(const Data& data) noexcept
            {
                requireBindings<Data>();
#if SUB0PUB_AUDIT
                (auditBound<Data>(Bound, audit::boundSignature<Bound>()), ...);
#endif
                StaticWiring<Bound...>::publish(data);
            }

        private:
#if SUB0PUB_AUDIT
            /// Record, for the audit, that this publication is delivered to a bound receiver that handles Data
            template<class Data, class Binding>
            static void auditBound(Binding* binding, const char* objectSignature) noexcept
            {
                if constexpr (handles_v<Binding, Data>)
                    audit::Ledger<Data>::bound(&wiring::receiver(*binding),
                                               audit::typeSignature<wiring::receiver_t<Binding>>(), objectSignature);
            }
#endif

            /// The compile-time checks of one publication; none leaves code behind
            template<class Data>
            static constexpr void requireBindings() noexcept
            {
                // Capability routing finds no receive() on an incomplete type and would deliver nothing.
                // An error here: the publisher's translation unit must include the definition of every bound receiver
                static_assert(((sizeof(*Bound) > 0) && ...));

                // Without `override` nothing else checks a wired receiver's signature (docs/DESIGN.md, K14)
                static_assert(((!std::is_base_of_v<Subscribe<Data>, wiring::receiver_t<std::remove_pointer_t<decltype(Bound)>>>
                                || handles_v<std::remove_pointer_t<decltype(Bound)>, Data>) && ...),
                              "sub0pub: a receiver listed in StaticTo derives from Subscribe<Data> but has no receive(const Data&)");

                static_assert(config_t<Data>::filter
                              || !(wiring::HasFilter<wiring::receiver_t<std::remove_pointer_t<decltype(Bound)>>, Data> || ...),
                              "sub0pub: a receiver listed in StaticTo declares filter(), but its Data type is not configured "
                              "with sub0::Filter (SUB0PUB_FILTER)");

                // A const or throwing filter() is called here and ignored by the broker, which calls its own virtual
                static_assert(((!std::is_base_of_v<Subscribe<Data>, wiring::receiver_t<std::remove_pointer_t<decltype(Bound)>>>
                                || !wiring::HasFilter<wiring::receiver_t<std::remove_pointer_t<decltype(Bound)>>, Data>
                                || BrokerFilter<wiring::receiver_t<std::remove_pointer_t<decltype(Bound)>>, Data>) && ...),
                              "sub0pub: a receiver listed in StaticTo declares a filter() the runtime broker would not call; "
                              "write it as `bool filter(const Data&) noexcept` so the type filters the same way on both");

                static_assert(Open || config_t<Data>::noReceivers == NoReceivers::Allow || cReceivers<Data> != 0U,
                              "sub0pub: no receiver listed in this Data type's StaticTo can receive it, so the publication "
                              "would reach nobody: list a receiver, or configure the type with sub0::AllowNoReceivers");
            }
        };
    } // END: detail
} // END: sub0

#endif // CROG_SUB0PUB_WIRING_STATIC_TOPOLOGY_HPP
