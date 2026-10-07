#pragma once

/** @file wiring_report.hpp
 *  Spike: a guided route from runtime subscription to static wiring.
 *
 *  Write the application the easy way, over the runtime broker; configure the message types of interest with
 *  RecordWiring; run it. At exit it prints, per type, which receivers subscribed, in what order, how many
 *  deliveries each took, and whether that set ever changed while the type was being published. A type whose set
 *  was fixed is a candidate for StaticTo<>, and the report gives the list in delivery order:
 *
 *      struct Reading { int celsius; using sub0_config = sub0::config<sub0::spike::RecordWiring>; };
 *
 *  It is a diagnostic mode, not a production broker: it stands in for the library broker's default configuration
 *  (Direct dispatch, global table, no publish context, no lock) through the public Implementation<> option, and
 *  it needs RTTI to name the receivers.
 *
 *  Known limitation: MSVC 19.51 names a receiver by its Subscribe<T> base when the translation unit that publishes
 *  cannot see the receiver's class (typeid through a pointer to a class-template base; reproduced without
 *  Sub0Pub). clang-cl names the receiver in every layout. The addresses and the order are right on both, and they
 *  are what identify the objects to list.
 */

#include <cstdint>
#include <cstdio>
#include <type_traits>
#include <typeinfo>

#include "sub0pub/broker.hpp"
#include "sub0pub_spike/topology.hpp"

namespace sub0::spike
{
    namespace detail
    {
        /** One message type's entry in the report. */
        struct WiringRecord
        {
            void (*print)(std::FILE* out) noexcept;
            WiringRecord* next; // non-owning; records have static storage
        };

        /** The report: every recorded message type, printed when the program exits. */
        struct WiringReport
        {
            WiringRecord* head = nullptr; // non-owning
            WiringRecord* tail = nullptr; // non-owning

            void add(WiringRecord& record) noexcept
            {
                (tail != nullptr ? tail->next : head) = &record;
                tail = &record;
            }

            ~WiringReport()
            {
                if (head == nullptr)
                    return;
                std::fputs("sub0pub wiring report (message types configured with RecordWiring)\n", stdout);
                for (const WiringRecord* record = head; record != nullptr; record = record->next)
                    record->print(stdout);
            }
        };

        /** The process's report.
         *  A function-local static on purpose: it is created by the first subscription, which may be the
         *  constructor of a receiver with static storage, and is therefore destroyed after that receiver, so the
         *  report sees every receiver leave. It is write-only diagnostic state; nothing reads it back.
         */
        inline WiringReport& wiringReport() noexcept
        {
            static WiringReport report;
            return report;
        }

        /** A runtime broker with the library broker's default dispatch that records who subscribed and when. */
        template<class Data, class Config>
        class RecordingBroker
        {
            static_assert(Config::context == Context::None && Config::storage == Storage::Global &&
                          std::is_same_v<typename Config::Lock, NoLock>,
                          "sub0pub spike: RecordingBroker stands in for the default configuration only");

            static constexpr uint32_t cObservations = Config::capacity * 2U; ///< receivers remembered per type

            struct Observation
            {
                const void* address;        // non-owning; printed, never dereferenced
                const std::type_info* type; // learned at the first delivery: the receiver is still under construction when it subscribes
                uint64_t deliveries;
                uint64_t joinedAt;          // publications of the type before it subscribed
                uint64_t leftAt;            // publications of the type before it left
                bool present;
            };

        public:
            SubscribeResult trySubscribe(sub0::Subscribe<Data>* subscriber) noexcept
            {
                if (!linked_)
                {
                    linked_ = true;
                    wiringReport().add(record_);
                }
                if (count_ >= Config::capacity)
                    return SubscribeResult::CapacityExceeded;
                if (observed_ < cObservations)
                {
                    observations_[observed_] = Observation{subscriber, nullptr, 0U, publications_, 0U, true};
                    observation_[count_] = observed_++;
                }
                else
                {
                    observation_[count_] = cObservations; // not remembered
                    truncated_ = true;
                }
                entries_[count_++] = subscriber;
                return SubscribeResult::Subscribed;
            }

            void disconnect(sub0::Subscribe<Data>* subscriber) noexcept
            {
                for (uint32_t i = 0; i < count_; ++i)
                    if (entries_[i] == subscriber)
                    {
                        if (observation_[i] < cObservations)
                        {
                            observations_[observation_[i]].present = false;
                            observations_[observation_[i]].leftAt = publications_;
                        }
                        for (uint32_t j = i + 1; j < count_; ++j)
                        {
                            entries_[j - 1] = entries_[j];
                            observation_[j - 1] = observation_[j];
                        }
                        --count_;
                        return;
                    }
            }

            void publish(const Data& data, const void* = nullptr, PublishReport* = nullptr) const noexcept
            {
                if (count_ == 0)
                    ++unheard_;
                for (uint32_t i = 0; i < count_; ++i)
                {
                    if (observation_[i] < cObservations)
                    {
                        Observation& seen = observations_[observation_[i]];
                        if (seen.type == nullptr)
                            seen.type = &typeid(*entries_[i]);
                        ++seen.deliveries;
                    }
                    kit::deliverAt<Data, false>(entries_[i], data);
                }
                ++publications_;
            }

            void cancel() const noexcept {}

            /** @return The number of subscribers registered at this moment. */
            [[nodiscard]] static uint32_t receivers() noexcept { return count_; }

        private:
            /// Whether every receiver subscribed before the first publication and stayed until after the last
            static bool fixedSet() noexcept
            {
                for (uint32_t i = 0; i < observed_; ++i)
                {
                    const Observation& seen = observations_[i];
                    if (seen.joinedAt != 0U || (!seen.present && seen.leftAt != publications_))
                        return false;
                }
                return !truncated_;
            }

            static void print(std::FILE* out) noexcept
            {
                std::fprintf(out, "  %s: %llu publications", typeid(Data).name(), static_cast<unsigned long long>(publications_));
                if (unheard_ != 0U)
                    std::fprintf(out, ", %llu reached nobody", static_cast<unsigned long long>(unheard_));
                std::fputc('\n', out);
                for (uint32_t i = 0; i < observed_; ++i)
                {
                    const Observation& seen = observations_[i];
                    std::fprintf(out, "    %u. %s at %p: %llu deliveries", i + 1U,
                                 seen.type != nullptr ? seen.type->name() : "(never called)", seen.address,
                                 static_cast<unsigned long long>(seen.deliveries));
                    if (seen.joinedAt != 0U)
                        std::fprintf(out, ", joined after publication %llu", static_cast<unsigned long long>(seen.joinedAt));
                    if (!seen.present && seen.leftAt != publications_)
                        std::fprintf(out, ", left after publication %llu", static_cast<unsigned long long>(seen.leftAt));
                    std::fputc('\n', out);
                }
                if (observed_ == 0U)
                    std::fputs("    nobody ever subscribed: AllowNoReceivers if that is intended\n", out);
                else if (publications_ == 0U)
                    std::fputs("    never published\n", out);
                else if (fixedSet())
                    std::fprintf(out, "    the set never changed: a candidate for StaticTo<> listing these %u objects in this order, "
                                      "if they have static storage\n", observed_);
                else
                    std::fputs("    receivers joined or left while it was being published: keep it brokered\n", out);
            }

            inline static uint32_t count_ = 0;
            inline static sub0::Subscribe<Data>* entries_[Config::capacity] = {}; // non-owning; subscribers disconnect themselves
            inline static uint32_t observation_[Config::capacity] = {};           // table slot -> index into observations_
            inline static Observation observations_[cObservations] = {};
            inline static uint32_t observed_ = 0;
            inline static uint64_t publications_ = 0;
            inline static uint64_t unheard_ = 0;
            inline static bool truncated_ = false;
            inline static bool linked_ = false;
            inline static WiringRecord record_{&RecordingBroker::print, nullptr};
        };
    }

    /** Configuration option: record this message type's receivers and print them at exit (see the file comment). */
    using RecordWiring = Implementation<detail::RecordingBroker>;
} // namespace sub0::spike
