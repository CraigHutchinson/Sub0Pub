#pragma once

/** @file subscription.hpp
 *  Spike "bus parameter" / "port member": runtime subscription for a receiver written as a plain class, so the
 *  class that static wiring binds can also join the runtime broker without deriving from Subscribe<T>.
 *  Supersedes the earlier MemberSubscription / StaticMemberSubscription spikes: one owner reference serves
 *  every subscribed type, and receive() is found by capability, as static wiring finds it.
 */

#include "sub0pub/broker.hpp"

namespace sub0::spike
{
    namespace detail
    {
        /** One runtime subscription that forwards to the owner held by the Subscription it is a base of. */
        template<class Derived, class Data>
        class Forwarding : public sub0::Subscribe<Data>
        {
        private:
            void receive(const Data& data) noexcept final { static_cast<Derived&>(*this).owner_.receive(data); }
        };
    }

    /** Subscribes a plain receiver to the runtime broker for each of Datas for as long as it lives.
     * @tparam Owner  A class with a non-virtual `receive(const Data&) noexcept` for each of Datas.
     * @note Registers in its constructor, like Subscribe<T>; a locked (concurrent) type would need an explicit
     *       trySubscribe() step, which this spike does not expose.
     */
    template<class Owner, class... Datas>
    class Subscription final : private detail::Forwarding<Subscription<Owner, Datas...>, Datas>...
    {
    public:
        /** Subscribes owner to each of Datas.
         * @param owner  The receiver; it must outlive this subscription.
         */
        explicit Subscription(Owner& owner) noexcept : owner_(owner) {}

    private:
        template<class, class> friend class detail::Forwarding;

        Owner& owner_; // non-owning; outlives the subscription by contract
    };
} // namespace sub0::spike
