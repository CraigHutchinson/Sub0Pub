#pragma once

/** @file no_receivers.hpp
 *  Spike: a publication that reaches no receiver is a reported failure by default.
 *
 *  For a StaticTo<> type the rule is enforced at compile time (topology.hpp). For a brokered type it has to be a
 *  run-time check in the broker's publish, and the question is what that costs every publication that does have
 *  receivers. The library broker is not modified here, so this header provides a small broker of its own through
 *  the public Implementation<> option, in two forms that differ only in the check:
 *
 *      using sub0_config = sub0::config<sub0::spike::ReportNoReceivers>;   // reports an empty publication
 *      using sub0_config = sub0::config<sub0::spike::SilentDirect>;        // the same broker without the check
 *
 *  Measuring one against the other isolates the check; measuring SilentDirect against the library broker shows
 *  the stand-in dispatches as the library does in its default configuration. A type opts out with
 *  AllowNoReceivers, and a call site decides for itself with tryPublish() or receiverCount().
 */

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <type_traits>

#include "sub0pub/broker.hpp"
#include "sub0pub_spike/topology.hpp"

/** Action when a publication reaches no receiver and its type has not opted out with AllowNoReceivers.
 * @param what  Null-terminated description of the failure
 * Default asserts, then aborts, as the library's other contract violations do. Override to log or count instead;
 * if it returns, the publication simply had no effect.
 */
#ifndef SUB0PUB_SPIKE_NO_RECEIVERS
#define SUB0PUB_SPIKE_NO_RECEIVERS(what) do { assert(!(what)); std::abort(); } while(false)
#endif

namespace sub0::spike
{
    namespace detail
    {
        /** A runtime broker with the library broker's default dispatch (Direct, global table, no publish context,
         *  no lock), optionally reporting a publication that reaches no subscriber.
         * @tparam Report  Whether publish() reports an empty table; false is the control for measuring the check.
         */
        template<class Data, class Config, bool Report>
        class DirectBroker
        {
            static_assert(Config::context == Context::None && Config::storage == Storage::Global &&
                          std::is_same_v<typename Config::Lock, NoLock>,
                          "sub0pub spike: DirectBroker stands in for the default configuration only");

        public:
            SubscribeResult trySubscribe(sub0::Subscribe<Data>* subscriber) noexcept
            {
                if (count_ >= Config::capacity)
                    return SubscribeResult::CapacityExceeded;
                entries_[count_++] = subscriber;
                return SubscribeResult::Subscribed;
            }

            void disconnect(sub0::Subscribe<Data>* subscriber) noexcept
            {
                for (uint32_t i = 0; i < count_; ++i)
                    if (entries_[i] == subscriber)
                    {
                        for (uint32_t j = i + 1; j < count_; ++j)
                            entries_[j - 1] = entries_[j];
                        --count_;
                        return;
                    }
            }

            void publish(const Data& data, const void* = nullptr, PublishReport* = nullptr) const noexcept
            {
                if constexpr (Report && !allowsNoReceivers<Config>)
                {
                    // The report takes the place of the loop's own entry test, so a publication that has
                    // receivers runs the same instructions with or without it
                    if (count_ == 0) [[unlikely]]
                    {
                        SUB0PUB_SPIKE_NO_RECEIVERS("sub0pub: a message was published and no receiver is subscribed to its type");
                        return;
                    }
                    uint32_t i = 0;
                    do
                        kit::deliverAt<Data, false>(entries_[i], data);
                    while (++i < count_);
                }
                else
                    for (uint32_t i = 0; i < count_; ++i)
                        kit::deliverAt<Data, false>(entries_[i], data);
            }

            /** Publishes without reporting an empty table.
             * @param data  The message.
             * @return The number of subscribers it was delivered to.
             */
            [[nodiscard]] uint32_t publishCounted(const Data& data) const noexcept
            {
                uint32_t delivered = 0;
                for (; delivered < count_; ++delivered)
                    kit::deliverAt<Data, false>(entries_[delivered], data);
                return delivered;
            }

            void cancel() const noexcept {}

            /** @return The number of subscribers registered at this moment. */
            [[nodiscard]] static uint32_t receivers() noexcept { return count_; }

        private:
            inline static uint32_t count_ = 0;
            inline static sub0::Subscribe<Data>* entries_[Config::capacity] = {}; // non-owning; subscribers disconnect themselves
        };
    }

    /** Broker that reports a publication reaching no subscriber (see ReportNoReceivers). */
    template<class Data, class Config>
    using ReportingBroker = detail::DirectBroker<Data, Config, true>;

    /** The same broker without the report: the control for measuring it (see SilentDirect). */
    template<class Data, class Config>
    using SilentBroker = detail::DirectBroker<Data, Config, false>;

    /** Configuration option: report a publication of this type that reaches no receiver. */
    using ReportNoReceivers = Implementation<ReportingBroker>;

    /** Configuration option: the stand-in broker with no report, for A/B measurement only. */
    using SilentDirect = Implementation<SilentBroker>;
} // namespace sub0::spike
