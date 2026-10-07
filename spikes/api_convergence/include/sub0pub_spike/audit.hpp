#pragma once

/** @file audit.hpp
 *  Spike: the audit. One recording of what a run actually published and delivered, reported at exit.
 *
 *  Several checks in this spike ask the same question from different sides: did this publication reach anybody,
 *  was this receiver ever called, is this subscriber in its type's list, did every translation unit agree on a
 *  type's topology, could this type be wired statically. The audit answers all of them from one ledger per
 *  message type, fed by both delivery paths (the runtime broker and a StaticTo<> list), and prints:
 *
 *    findings   publications that reached no receiver, and which publisher made them
 *               subscribers that never received a message, or whose type was never published
 *               subscribers of a wired type that its StaticTo<> does not list
 *               a type published with different topologies in different translation units
 *               subscriptions refused because the table was full
 *    per type   its publishers and their counts; its receivers in delivery order with their deliveries and when
 *               they joined or left; the table's peak against its capacity; and whether the set of receivers
 *               ever changed, which is what makes a brokered type a candidate for StaticTo<>
 *
 *  It is a build mode, not a source change: compile the program with
 *
 *      -DSUB0PUB_CONFIG_HEADER="sub0pub_spike/audit_build.hpp"
 *
 *  A test asks for the count with auditFindings(), or sets SUB0PUB_SPIKE_AUDIT_EXIT to fail a run that has any.
 *
 *  Limits of the spike: the brokered side stands in for the library broker's default configuration (Direct
 *  dispatch, global table, no publish context, no lock) through the public Implementation<> option; it is
 *  single-threaded; it remembers a fixed number of receivers and publishers per type; it needs RTTI; and a
 *  publication made with sub0::publish() directly is counted but not attributed to a publisher.
 */

#include <cstdint>
#include <cstdio>
#include <type_traits>
#include <typeinfo>

#include "sub0pub/broker.hpp"

/** Whether this is an audit build. Set by sub0pub_spike/audit_build.hpp; do not define it by hand. */
#ifndef SUB0PUB_SPIKE_AUDIT
#define SUB0PUB_SPIKE_AUDIT false
#endif

/** Action after the audit has been printed at exit.
 * @param findings  The number of findings reported
 * Default does nothing. A test or CI build can fail the run instead, for example
 * `#define SUB0PUB_SPIKE_AUDIT_EXIT(findings) do { if ((findings) != 0U) std::_Exit(1); } while(false)`.
 */
#ifndef SUB0PUB_SPIKE_AUDIT_EXIT
#define SUB0PUB_SPIKE_AUDIT_EXIT(findings) ((void)(findings))
#endif

namespace sub0::spike
{
    namespace detail
    {
        /** One message type's entry in the audit. */
        struct AuditRecord
        {
            uint32_t (*findings)(std::FILE* out) noexcept; ///< prints the type's findings when out is non-null; returns their count
            void (*detail)(std::FILE* out) noexcept;       ///< prints the type's publishers and receivers
            void (*learn)() noexcept;                      ///< names the receivers its broker holds now; null if it has none
            AuditRecord* next;                             // non-owning; records have static storage
        };

        /** The process's audit: every recorded message type, printed when the program exits. */
        struct Audit
        {
            AuditRecord* head = nullptr; // non-owning
            AuditRecord* tail = nullptr; // non-owning

            void add(AuditRecord& record) noexcept
            {
                (tail != nullptr ? tail->next : head) = &record;
                tail = &record;
            }

            /** Names every receiver that a broker currently holds and that has not been delivered to yet.
             *  A subscriber's type cannot be read while it is being constructed, which is when it joins, so the
             *  audit reads it at the first delivery, or here: whenever any subscriber joins or leaves, and whenever
             *  the findings are asked for. A receiver that is created last and never called stays unnamed.
             */
            void learn() const noexcept
            {
                for (const AuditRecord* record = head; record != nullptr; record = record->next)
                    if (record->learn != nullptr)
                        record->learn();
            }

            /** @return The findings so far, without printing. */
            [[nodiscard]] uint32_t findings() const noexcept
            {
                learn();
                uint32_t count = 0;
                for (const AuditRecord* record = head; record != nullptr; record = record->next)
                    count += record->findings(nullptr);
                return count;
            }

            ~Audit()
            {
                if (head == nullptr)
                    return;
                uint32_t count = 0; // not findings(): by now the receivers a broker held may be gone
                for (const AuditRecord* record = head; record != nullptr; record = record->next)
                    count += record->findings(nullptr);
                std::fprintf(stdout, "sub0pub audit: %u finding%s\n", count, count == 1U ? "" : "s");
                for (const AuditRecord* record = head; record != nullptr; record = record->next)
                    (void)record->findings(stdout);
                for (const AuditRecord* record = head; record != nullptr; record = record->next)
                    record->detail(stdout);
                std::fflush(stdout);
                SUB0PUB_SPIKE_AUDIT_EXIT(count);
            }
        };

        /** The process's audit.
         *  A function-local static on purpose: it is created by the first recorded event, which may be the
         *  constructor of a receiver with static storage, and is therefore destroyed after that receiver, so the
         *  report sees every receiver leave. It is write-only diagnostic state; nothing depends on it for behaviour.
         */
        inline Audit& audit() noexcept
        {
            static Audit instance;
            return instance;
        }

        /** The text a compiler gives a function template whose argument is an object's address: it contains the
         *  object's name, which is how the audit names the receivers of a StaticTo<> list. */
        template<auto* Bound>
        const char* boundSignature() noexcept
        {
#if defined(__clang__) || defined(__GNUC__)
            return __PRETTY_FUNCTION__;
#else
            return __FUNCSIG__;
#endif
        }

        /// Prints the object name out of a boundSignature(): the identifier after its last '&'
        inline void printBoundName(std::FILE* out, const char* signature) noexcept
        {
            const char* name = nullptr;
            for (const char* c = signature; *c != '\0'; ++c)
                if (*c == '&')
                    name = c + 1;
            if (name == nullptr)
            {
                std::fputs(signature, out);
                return;
            }
            for (; (*name >= 'a' && *name <= 'z') || (*name >= 'A' && *name <= 'Z') || (*name >= '0' && *name <= '9') ||
                   *name == '_' || *name == ':'; ++name)
                std::fputc(*name, out);
        }

        /** Everything the audit knows about one message type. Independent of the type's configuration, so every
         *  translation unit feeds the same ledger even when they disagree about the topology. */
        template<class Data>
        class AuditLedger
        {
        public:
            static constexpr uint32_t cReceivers = 16U;  ///< receivers remembered per type
            static constexpr uint32_t cPublishers = 8U;  ///< publisher types remembered per type
            static constexpr uint32_t cUnknown = ~0U;    ///< a reach the audit could not determine

            /// A runtime subscriber joined; the handle identifies it in left() and delivered()
            static uint32_t joined(const void* address) noexcept
            {
                link();
                if (count_ >= cReceivers)
                {
                    truncated_ = true;
                    return cReceivers;
                }
                receivers_[count_] = Receiver{address, nullptr, nullptr, 0U, publications_, 0U, true, false};
                return count_++;
            }

            static void left(uint32_t handle) noexcept
            {
                if (handle >= cReceivers)
                    return;
                receivers_[handle].present = false;
                receivers_[handle].leftAt = publications_;
            }

            static void delivered(uint32_t handle, const std::type_info& type) noexcept
            {
                if (handle >= cReceivers)
                    return;
                receivers_[handle].type = &type;
                ++receivers_[handle].deliveries;
            }

            /// The type of a receiver that has joined, read outside a delivery; a delivery's reading wins
            static void learned(uint32_t handle, const std::type_info& type) noexcept
            {
                if (handle < cReceivers && receivers_[handle].type == nullptr)
                    receivers_[handle].type = &type;
            }

            /// How the audit asks this type's broker to name the receivers it holds
            static void learner(void (*learn)() noexcept) noexcept { record_.learn = learn; }

            /// A runtime subscription was refused because the table was full
            static void refused() noexcept
            {
                link();
                ++refused_;
            }

            /// The runtime table's size after a subscription
            static void table(uint32_t size, uint32_t capacity) noexcept
            {
                capacity_ = capacity;
                if (size > peak_)
                    peak_ = size;
            }

            /// A delivery to a receiver listed in the type's StaticTo<>; `key` is its Subscribe<Data> base, or the object
            static void wired(const void* key, const std::type_info& type, const char* signature) noexcept
            {
                link();
                uint32_t i = 0;
                while (i < count_ && receivers_[i].address != key)
                    ++i;
                if (i == count_)
                {
                    if (count_ >= cReceivers)
                    {
                        truncated_ = true;
                        return;
                    }
                    receivers_[count_++] = Receiver{key, nullptr, nullptr, 0U, 0U, 0U, true, false};
                }
                Receiver& receiver = receivers_[i];
                receiver.type = &type;
                receiver.bound = signature;
                receiver.present = true; // a StaticFirst receiver registers with the broker and leaves again: it is here
                ++receiver.deliveries;
            }

            /// A subscriber of a wired type was constructed; unlisted ones are never called
            static void wiredSubscriber(const void* address, bool listed) noexcept
            {
                link();
                if (listed)
                    return;
                if (count_ >= cReceivers)
                {
                    truncated_ = true;
                    return;
                }
                receivers_[count_++] = Receiver{address, nullptr, nullptr, 0U, publications_, 0U, true, true};
            }

            /// The topology a translation unit published the type with
            static void topology(uint32_t fingerprint) noexcept
            {
                if (topology_ == 0U)
                    topology_ = fingerprint;
                else if (topology_ != fingerprint)
                    mixed_ = true;
            }

            /// One publication: by whom (null if not known) and how many receivers it reached (cUnknown if not known)
            static void published(const std::type_info* publisher, uint32_t reached, bool unheardAllowed) noexcept
            {
                link();
                unheardAllowed_ = unheardAllowed_ || unheardAllowed;
                uint32_t i = 0;
                while (i < publisherCount_ && !samePublisher(publishers_[i].type, publisher))
                    ++i;
                if (i == publisherCount_ && publisherCount_ < cPublishers)
                    publishers_[publisherCount_++] = Publisher{publisher, 0U, 0U};
                if (i < publisherCount_)
                {
                    ++publishers_[i].publications;
                    publishers_[i].unheard += (reached == 0U) ? 1U : 0U;
                }
                ++publications_;
                unheard_ += (reached == 0U) ? 1U : 0U;
            }

            /// Whether the publication now being dispatched was already recorded by publish(); see AuditAccounted
            static bool& accounted() noexcept { return accounted_; }

        private:
            struct Receiver
            {
                const void* address;        // non-owning; printed and compared, never dereferenced
                const std::type_info* type; // learned at the first delivery: a subscriber is still under construction when it joins
                const char* bound;          // the boundSignature() of a listed receiver, else null
                uint64_t deliveries;
                uint64_t joinedAt;          // publications of the type before it joined
                uint64_t leftAt;            // publications of the type before it left
                bool present;
                bool unlisted;              // a subscriber of a wired type that its StaticTo<> does not name
            };

            struct Publisher
            {
                const std::type_info* type; // null: published through sub0::publish() directly
                uint64_t publications;
                uint64_t unheard;
            };

            static bool samePublisher(const std::type_info* a, const std::type_info* b) noexcept
            {
                return a == b || (a != nullptr && b != nullptr && *a == *b);
            }

            static void link() noexcept
            {
                if (!linked_)
                {
                    linked_ = true;
                    audit().add(record_);
                }
            }

            static bool leftEarly(const Receiver& receiver) noexcept
            {
                return !receiver.present && receiver.leftAt != publications_;
            }

            /// Whether every receiver was there for every publication
            static bool fixedSet() noexcept
            {
                for (uint32_t i = 0; i < count_; ++i)
                    if (receivers_[i].joinedAt != 0U || leftEarly(receivers_[i]) || receivers_[i].unlisted)
                        return false;
                return !truncated_;
            }

            static void describe(std::FILE* out, const Receiver& receiver) noexcept
            {
                if (receiver.bound != nullptr)
                {
                    printBoundName(out, receiver.bound);
                    std::fprintf(out, " (%s)", receiver.type->name());
                }
                else
                    std::fputs(receiver.type != nullptr ? receiver.type->name() : "a subscriber", out);
                std::fprintf(out, " at %p", receiver.address);
            }

            static uint32_t findings(std::FILE* out) noexcept
            {
                const char* const name = typeid(Data).name();
                uint32_t count = 0;

                if (unheard_ != 0U && !unheardAllowed_)
                {
                    ++count;
                    if (out != nullptr)
                    {
                        std::fprintf(out, "  %s: %llu of %llu publications reached no receiver", name,
                                     static_cast<unsigned long long>(unheard_), static_cast<unsigned long long>(publications_));
                        for (uint32_t i = 0; i < publisherCount_; ++i)
                            if (publishers_[i].unheard != 0U)
                                std::fprintf(out, "; %llu by %s", static_cast<unsigned long long>(publishers_[i].unheard),
                                             publishers_[i].type != nullptr ? publishers_[i].type->name() : "an unrecorded publisher");
                        std::fputc('\n', out);
                    }
                }
                if (mixed_)
                {
                    ++count;
                    if (out != nullptr)
                        std::fprintf(out, "  %s: published with different topologies in different translation units; one of them "
                                          "missed its configuration\n", name);
                }
                if (refused_ != 0U)
                {
                    ++count;
                    if (out != nullptr)
                        std::fprintf(out, "  %s: %u subscription%s refused, the table of %u was full\n", name, refused_,
                                     refused_ == 1U ? "" : "s", capacity_);
                }
                for (uint32_t i = 0; i < count_; ++i)
                {
                    const Receiver& receiver = receivers_[i];
                    const bool neverReceived = receiver.deliveries == 0U && publications_ != 0U;
                    if (!receiver.unlisted && !neverReceived)
                        continue;
                    ++count;
                    if (out == nullptr)
                        continue;
                    std::fprintf(out, "  %s: ", name);
                    describe(out, receiver);
                    if (receiver.unlisted)
                        std::fputs(" subscribes to it but is not in its StaticTo<> list, so it is never called\n", out);
                    else if (receiver.joinedAt == publications_)
                        std::fputs(" never received it: it subscribed after the last publication\n", out);
                    else if (!receiver.present && receiver.leftAt == 0U)
                        std::fputs(" never received it: it left before the first publication\n", out);
                    else
                        std::fprintf(out, " never received it: it was subscribed only between publications %llu and %llu\n",
                                     static_cast<unsigned long long>(receiver.joinedAt), static_cast<unsigned long long>(receiver.leftAt));
                }
                if (publications_ == 0U && count_ != 0U)
                {
                    ++count;
                    if (out != nullptr)
                        std::fprintf(out, "  %s: never published, although %u receiver%s subscribed to it\n", name, count_,
                                     count_ == 1U ? "" : "s");
                }
                return count;
            }

            static void detail(std::FILE* out) noexcept
            {
                std::fprintf(out, "  %s: %llu publication%s", typeid(Data).name(), static_cast<unsigned long long>(publications_),
                             publications_ == 1U ? "" : "s");
                if (unheard_ != 0U && unheardAllowed_)
                    std::fprintf(out, " (%llu reached nobody, which its configuration allows)", static_cast<unsigned long long>(unheard_));
                if (capacity_ != 0U)
                    std::fprintf(out, "; runtime table peaked at %u of %u", peak_, capacity_);
                std::fputc('\n', out);
                for (uint32_t i = 0; i < publisherCount_; ++i)
                    std::fprintf(out, "    published by %s: %llu\n",
                                 publishers_[i].type != nullptr ? publishers_[i].type->name() : "an unrecorded publisher (sub0::publish)",
                                 static_cast<unsigned long long>(publishers_[i].publications));
                for (uint32_t i = 0; i < count_; ++i)
                {
                    const Receiver& receiver = receivers_[i];
                    std::fprintf(out, "    %u. ", i + 1U);
                    describe(out, receiver);
                    std::fprintf(out, ": %llu deliver%s", static_cast<unsigned long long>(receiver.deliveries),
                                 receiver.deliveries == 1U ? "y" : "ies");
                    if (receiver.unlisted)
                        std::fputs(", not listed", out);
                    else if (receiver.bound != nullptr)
                        std::fputs(", wired", out);
                    if (receiver.joinedAt != 0U && !receiver.unlisted)
                        std::fprintf(out, ", joined after publication %llu", static_cast<unsigned long long>(receiver.joinedAt));
                    if (leftEarly(receiver))
                        std::fprintf(out, ", left after publication %llu", static_cast<unsigned long long>(receiver.leftAt));
                    std::fputc('\n', out);
                }
                if (truncated_)
                    std::fprintf(out, "    more than %u receivers: the rest were not recorded\n", cReceivers);
                if (count_ != 0U && publications_ != 0U && capacity_ != 0U)
                    std::fputs(fixedSet() ? "    the receivers never changed: a candidate for StaticTo<>, in this order, if they have static storage\n"
                                          : "    receivers joined or left while it was being published: keep it brokered\n", out);
            }

            inline static Receiver receivers_[cReceivers] = {};
            inline static Publisher publishers_[cPublishers] = {};
            inline static uint32_t count_ = 0;
            inline static uint32_t publisherCount_ = 0;
            inline static uint64_t publications_ = 0;
            inline static uint64_t unheard_ = 0;
            inline static uint32_t peak_ = 0;
            inline static uint32_t capacity_ = 0;
            inline static uint32_t refused_ = 0;
            inline static uint32_t topology_ = 0;
            inline static bool mixed_ = false;
            inline static bool unheardAllowed_ = false;
            inline static bool truncated_ = false;
            inline static bool accounted_ = false;
            inline static bool linked_ = false;
            inline static AuditRecord record_{&AuditLedger::findings, &AuditLedger::detail, nullptr, nullptr};
        };

        /** Marks, for its lifetime, that the publication being dispatched was recorded by publish(), with its
         *  publisher; the audit broker then leaves the count alone. */
        template<class Data>
        class AuditAccounted
        {
        public:
            AuditAccounted() noexcept : previous_(AuditLedger<Data>::accounted()) { AuditLedger<Data>::accounted() = true; }
            ~AuditAccounted() { AuditLedger<Data>::accounted() = previous_; }
            AuditAccounted(const AuditAccounted&) = delete;
            AuditAccounted& operator=(const AuditAccounted&) = delete;

        private:
            bool previous_;
        };

        /** A runtime broker with the library broker's default dispatch that feeds the audit. */
        template<class Data, class Config>
        class AuditBroker
        {
            static_assert(Config::context == Context::None && Config::storage == Storage::Global &&
                          std::is_same_v<typename Config::Lock, NoLock>,
                          "sub0pub spike: the audit stands in for the default configuration only");

            using Ledger = AuditLedger<Data>;
            static constexpr bool cUnheardAllowed = requires { requires Config::allowNoReceivers; };

        public:
            SubscribeResult trySubscribe(sub0::Subscribe<Data>* subscriber) noexcept
            {
                if (count_ >= Config::capacity)
                {
                    Ledger::refused();
                    return SubscribeResult::CapacityExceeded;
                }
                audit().learn(); // before this one joins: it is still under construction
                Ledger::learner(&AuditBroker::learn);
                handles_[count_] = Ledger::joined(subscriber);
                entries_[count_++] = subscriber;
                Ledger::table(count_, Config::capacity);
                return SubscribeResult::Subscribed;
            }

            void disconnect(sub0::Subscribe<Data>* subscriber) noexcept
            {
                for (uint32_t i = 0; i < count_; ++i)
                    if (entries_[i] == subscriber)
                    {
                        Ledger::left(handles_[i]);
                        for (uint32_t j = i + 1; j < count_; ++j)
                        {
                            entries_[j - 1] = entries_[j];
                            handles_[j - 1] = handles_[j];
                        }
                        --count_;
                        audit().learn(); // after this one left: it is already partly destroyed
                        return;
                    }
            }

            void publish(const Data& data, const void* = nullptr, PublishReport* = nullptr) const noexcept
            {
                (void)publishCounted(data);
            }

            /** Publishes and reports how many subscribers the message was delivered to.
             * @param data  The message.
             * @return The number of subscribers called.
             */
            uint32_t publishCounted(const Data& data) const noexcept
            {
                if (!Ledger::accounted())
                    Ledger::published(nullptr, count_, cUnheardAllowed); // sub0::publish() directly: no publisher to name
                uint32_t delivered = 0;
                for (; delivered < count_; ++delivered)
                {
                    Ledger::delivered(handles_[delivered], typeOf(entries_[delivered]));
                    kit::deliverAt<Data, false>(entries_[delivered], data);
                }
                return delivered;
            }

            void cancel() const noexcept {}

            /** @return The number of subscribers registered at this moment. */
            [[nodiscard]] static uint32_t receivers() noexcept { return count_; }

        private:
            /// Names the receivers in the table that have not been delivered to yet
            static void learn() noexcept
            {
                for (uint32_t i = 0; i < count_; ++i)
                {
                    const std::type_info& type = typeOf(entries_[i]);
                    if (type != typeid(sub0::Subscribe<Data>)) // still only its base: under construction or destruction
                        Ledger::learned(handles_[i], type);
                }
            }

            static const std::type_info& typeOf(const sub0::Subscribe<Data>* subscriber) noexcept
            {
                // MSVC 19.51 evaluates typeid(*p) at compile time, naming the base, when p's class is a template
                // specialisation that has not been instantiated yet at this point; requiring its size first does that
                static_assert(sizeof(sub0::Subscribe<Data>) > 0);
                return typeid(*subscriber);
            }

            inline static uint32_t count_ = 0;
            inline static sub0::Subscribe<Data>* entries_[Config::capacity] = {}; // non-owning; subscribers disconnect themselves
            inline static uint32_t handles_[Config::capacity] = {};               // table slot -> the ledger's handle
        };
    }

    /** Configuration option: audit this brokered message type in a build that is not an audit build.
     *  Publishers are then not attributed; prefer the audit build (see the file comment). */
    using Audited = Implementation<detail::AuditBroker>;

    /** The number of findings the audit has recorded so far; 0 in a build that records nothing.
     *  Receivers that have not yet been destroyed are judged as they stand, so call it after the work under test.
     * @return Findings across every recorded message type.
     */
    [[nodiscard]] inline uint32_t auditFindings() noexcept { return detail::audit().findings(); }
} // namespace sub0::spike
