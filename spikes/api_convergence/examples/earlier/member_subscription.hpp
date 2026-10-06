#ifndef SUB0PUB_SPIKE_MEMBER_SUBSCRIPTION_HPP
#define SUB0PUB_SPIKE_MEMBER_SUBSCRIPTION_HPP

#include "sub0pub/broker.hpp"

#include <functional>
#include <type_traits>

namespace sub0::spike
{
    template<class Data, class Owner>
    class MemberSubscription
    {
        using Handler = void (Owner::*)(const Data&) noexcept;

        class Adapter final : public sub0::Subscribe<Data>
        {
        public:
            Adapter(Owner& owner, Handler handler) noexcept : owner_(owner), handler_(handler) {}

        private:
            void receive(const Data& data) noexcept override { std::invoke(handler_, owner_, data); }

            Owner& owner_;
            Handler handler_;
        };

    public:
        MemberSubscription(Owner& owner, Handler handler) noexcept : adapter_(owner, handler) {}

        bool isSubscribed() const noexcept { return adapter_.isSubscribed(); }
        sub0::SubscribeResult trySubscribe() noexcept { return adapter_.trySubscribe(); }
        void disconnect() noexcept { adapter_.disconnect(); }

    private:
        Adapter adapter_;
    };

    template<class Data, class Owner, auto Handler>
    class StaticMemberSubscription
    {
        static_assert(std::is_nothrow_invocable_r_v<void, decltype(Handler), Owner&, const Data&>);

        class Adapter final : public sub0::Subscribe<Data>
        {
        public:
            explicit Adapter(Owner& owner) noexcept : owner_(owner) {}

        private:
            void receive(const Data& data) noexcept override { std::invoke(Handler, owner_, data); }

            Owner& owner_;
        };

    public:
        explicit StaticMemberSubscription(Owner& owner) noexcept : adapter_(owner) {}

        bool isSubscribed() const noexcept { return adapter_.isSubscribed(); }
        sub0::SubscribeResult trySubscribe() noexcept { return adapter_.trySubscribe(); }
        void disconnect() noexcept { adapter_.disconnect(); }

    private:
        Adapter adapter_;
    };
} // namespace sub0::spike

#endif
