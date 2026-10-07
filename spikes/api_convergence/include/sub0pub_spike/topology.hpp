#pragma once

/** @file topology.hpp
 *  Spike "typed route": one participant API whose delivery is chosen per message type.
 *
 *  A message type's topology is one more axis of its per-type configuration (sub0pub/config.hpp), so it resolves
 *  through the same member alias / ADL / SUB0PUB_CONFIGURE / project-header chain as Capacity or Snapshot:
 *
 *      SUB0PUB_CONFIGURE(Reading, sub0::spike::StaticTo<&display, &audit>);   // fixed receivers, direct calls
 *      SUB0PUB_CONFIGURE(Reading, sub0::spike::StaticFirst<&display>);        // those first, then the broker
 *      (nothing)                                                             // the runtime broker
 *
 *  Participants are written once, in the runtime broker's spelling, against the aliases below:
 *
 *      struct Display : sub0::spike::Subscribe<Reading> { void receive(const Reading&) noexcept; };
 *      struct Sensor  : sub0::spike::Publish<Reading>   { void send() { sub0::spike::publish(*this, Reading{}); } };
 *
 *  Without a topology option the aliases *are* sub0::Subscribe / sub0::Publish, so the dynamic path is the
 *  existing one by construction. This layer leaves the library untouched: a production form would fold the
 *  same selection into sub0::Subscribe / sub0::Publish / sub0::publish themselves.
 */

#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <type_traits>

#include "sub0pub/broker.hpp"
#include "sub0pub/wiring.hpp"
#include "sub0pub_spike/audit.hpp"

/** Debug check that every subscriber of a StaticTo type is one of its bound receivers.
 *  A subscriber left out of a static topology is otherwise silently never called (compare DESIGN.md K14).
 */
#ifndef SUB0PUB_SPIKE_CHECK_BOUND
#if SUB0PUB_ASSERT && !defined(NDEBUG)
#define SUB0PUB_SPIKE_CHECK_BOUND true
#else
#define SUB0PUB_SPIKE_CHECK_BOUND false
#endif
#endif

/** Action when a subscriber of a StaticTo type is constructed without being listed (see SUB0PUB_SPIKE_CHECK_BOUND).
 * @param what  Null-terminated description of the violation
 * Default asserts, then aborts. Override to count or log instead, for example in a unit test that constructs a
 * receiver on its own; if it returns, construction continues and the receiver is simply never called.
 */
#ifndef SUB0PUB_SPIKE_UNLISTED_RECEIVER
#define SUB0PUB_SPIKE_UNLISTED_RECEIVER(what) do { assert(!(what)); std::abort(); } while(false)
#endif

/** Experiment knob: a StaticTo subscriber base keeps a virtual receive(), so existing `override` receivers compile
 *  unchanged. Measures the price of a zero-edit promotion (a vptr per subscriber base, a retained vtable).
 */
#ifndef SUB0PUB_SPIKE_STATIC_VIRTUAL
#define SUB0PUB_SPIKE_STATIC_VIRTUAL false
#endif

/** Lets a class with several empty bases stay empty on the MSVC ABI, which otherwise gives each extra one a byte. */
#if defined(_MSC_VER)
#define SUB0PUB_SPIKE_EMPTY_BASES __declspec(empty_bases)
#else
#define SUB0PUB_SPIKE_EMPTY_BASES
#endif

namespace sub0::spike
{
    namespace detail
    {
        /// Audit build: one delivery to a listed receiver, keyed as its subscriber base would key itself
        template<class Data, class Base, class Binding>
        void auditBound(Binding* binding, const char* signature) noexcept
        {
            using Receiver = sub0::detail::wiring::receiver_t<Binding>;
            if constexpr (handles_v<Binding, Data>)
            {
                Receiver& receiver = sub0::detail::wiring::receiver(*binding);
                const void* key = &receiver;
                if constexpr (std::is_base_of_v<Base, Receiver>)
                    key = static_cast<const Base*>(&receiver);
                AuditLedger<Data>::wired(key, typeid(Receiver), signature);
            }
        }

        /// Whether `subscriber` is the Base subobject of the receiver a wiring binding stands for.
        template<class Base, class Binding>
        bool isBound(Binding* binding, const Base* subscriber) noexcept
        {
            using Receiver = sub0::detail::wiring::receiver_t<Binding>;
            if constexpr (std::is_base_of_v<Base, Receiver>)
                return static_cast<const Base*>(&sub0::detail::wiring::receiver(*binding)) == subscriber;
            else
                return false;
        }
    }

    /** Topology of a message type delivered by the runtime broker: the default for every type. */
    struct Dynamic
    {
        static constexpr bool cHasStatic = false;
        static constexpr bool cHasDynamic = true;
    };

    /** Topology of a message type delivered to fixed receivers with static storage, by direct calls.
     * @tparam Bound  Addresses of the receivers (or of holders exposing them through get()), in delivery order.
     */
    template<auto*... Bound>
    struct Static : StaticWiring<Bound...>
    {
        static constexpr bool cHasStatic = true;
        static constexpr bool cHasDynamic = false;

        /** Reports whether a subscriber base subobject belongs to one of the bound receivers.
         * @param subscriber  The Subscribe<Data> base of a receiver.
         * @return True if that receiver is in Bound.
         */
        template<class Base>
        [[nodiscard]] static bool binds(const Base* subscriber) noexcept
        {
            return (detail::isBound(Bound, subscriber) || ...);
        }

        /** Rejects a bound receiver that declares filter() for a type that has not opted in to filtering.
         *  The runtime broker rejects such a subscriber unless the type is configured with sub0::Filter; a wired type
         *  follows the same rule, so one source is accepted or rejected alike whichever way the type is delivered.
         * @tparam Data           The message type being published.
         * @tparam FilterEnabled  Whether Data's configuration has sub0::Filter.
         */
        template<class Data, bool FilterEnabled>
        static constexpr void requireFilterOptIn() noexcept
        {
            static_assert(FilterEnabled || !(sub0::detail::wiring::HasFilter<
                              sub0::detail::wiring::receiver_t<std::remove_pointer_t<decltype(Bound)>>, Data> || ...),
                          "sub0pub: a receiver listed in StaticTo<> declares filter(), but its message type is not configured "
                          "with sub0::Filter");
        }

        /** The number of bound receivers that receive Data: a constant of the program. */
        template<class Data>
        static constexpr uint32_t cReceivers =
            (uint32_t(handles_v<std::remove_pointer_t<decltype(Bound)>, Data>) + ... + uint32_t(0));

        /** Records, in an audit build, one publication's delivery to each listed receiver of Data.
         * @tparam Base  The Subscribe<Data> base a subscriber of Data derives from.
         */
        template<class Data, class Base>
        static void auditDeliveries() noexcept
        {
            (detail::auditBound<Data, Base>(Bound, detail::boundSignature<Bound>()), ...);
        }

        /** Rejects a translation unit that publishes without the bound receivers' definitions.
         *  Capability routing would otherwise find no receive() on an incomplete type and silently deliver nothing.
         */
        static constexpr void requireComplete() noexcept
        {
            // An error here: include the definition of every receiver this type's StaticTo<> names
            static_assert(((sizeof(*Bound) > 0) && ...));
        }

        /** Rejects a bound receiver that subscribes to Data but cannot receive it.
         *  Without `override` nothing else checks a static receiver's signature: capability routing would skip a
         *  `receive(Data&)` or a misspelt overload without a diagnostic (DESIGN.md K14).
         * @tparam Base  The Subscribe<Data> base a subscriber of Data derives from.
         * @tparam Data  The message type being published.
         */
        template<class Base, class Data>
        static constexpr void requireHandlers() noexcept
        {
            static_assert(((!std::is_base_of_v<Base, sub0::detail::wiring::receiver_t<std::remove_pointer_t<decltype(Bound)>>>
                            || handles_v<std::remove_pointer_t<decltype(Bound)>, Data>) && ...),
                          "sub0pub: a receiver listed in StaticTo<> derives from Subscribe<Data> but has no receive(const Data&)");
        }
    };

    /** Topology of a message type delivered to fixed receivers first, then to runtime subscribers.
     * @tparam Bound  Addresses of the statically called receivers, in delivery order.
     */
    template<auto*... Bound>
    struct Bridged : Static<Bound...>
    {
        static constexpr bool cHasDynamic = true;
    };

    /** Configuration option: deliver this message type to fixed receivers only.
     * @tparam Bound  Addresses of receivers with static storage duration.
     * @note Every publisher's translation unit must see the option before it publishes the type, and the
     *       receiver definitions before the end of that translation unit (the same build contract as any
     *       other per-type configuration, DESIGN.md K6).
     */
    template<auto*... Bound>
    struct StaticTo
    {
        template<class Base>
        struct apply : Base
        {
            using topology = Static<Bound...>;
        };
    };

    /** Configuration option: deliver to fixed receivers first, then through the runtime broker.
     * @tparam Bound  Addresses of the receivers promoted to direct calls.
     */
    template<auto*... Bound>
    struct StaticFirst
    {
        template<class Base>
        struct apply : Base
        {
            using topology = Bridged<Bound...>;
        };
    };

    /** Configuration option: publishing this message type to no receiver at all is expected, not a failure.
     *  By default a publication that reaches nobody is reported: at compile time for StaticTo<>, at run time by a
     *  broker that checks (sub0pub_spike/no_receivers.hpp). Use this for a type whose receivers are optional: a
     *  diagnostic stream, a plug-in loaded at run time, a message compiled out with an empty StaticTo<>.
     */
    struct AllowNoReceivers
    {
        template<class Base>
        struct apply : Base
        {
            static constexpr bool allowNoReceivers = true;
        };
    };

    namespace detail
    {
        /// Whether a configuration opted out of the no-receivers report
        template<class Config>
        constexpr bool allowsNoReceivers = requires { requires Config::allowNoReceivers; };

        template<class Config>
        struct TopologyOf
        {
            using type = Dynamic;
        };

        template<class Config>
            requires requires { typename Config::topology; }
        struct TopologyOf<Config>
        {
            using type = typename Config::topology;
        };
    }

    /** The topology a message type's configuration selects; Dynamic unless it names StaticTo or StaticFirst. */
    template<class Data>
    using topology_t = typename detail::TopologyOf<config_t<Data>>::type;

    namespace detail
    {
        /** Subscriber base of a StaticTo type: no registration and no state, so the receiver is a plain class. */
        template<class Data>
        class StaticSubscribe
        {
        public:
#if SUB0PUB_SPIKE_AUDIT
            StaticSubscribe() noexcept { AuditLedger<Data>::wiredSubscriber(this, topology_t<Data>::binds(this)); }
#elif SUB0PUB_SPIKE_CHECK_BOUND
            StaticSubscribe() noexcept
            {
                if (!topology_t<Data>::binds(this))
                    SUB0PUB_SPIKE_UNLISTED_RECEIVER("sub0pub: this subscriber is not listed in its type's StaticTo<>");
            }
#else
            StaticSubscribe() = default; // trivial, so a receiver stays as cheap to construct as a plain class
#endif
            StaticSubscribe(const StaticSubscribe&) = delete;
            StaticSubscribe& operator=(const StaticSubscribe&) = delete;

#if SUB0PUB_SPIKE_STATIC_VIRTUAL
            /** Receives published Data; kept virtual only so `override` receivers compile unchanged. */
            virtual void receive(const Data& data) noexcept = 0;
#endif

        protected:
            ~StaticSubscribe() = default;
        };

        /** Subscriber base of a StaticFirst type: a runtime subscriber unless it is one of the bound receivers. */
        template<class Data>
        class BridgedSubscribe : public sub0::Subscribe<Data>
        {
        public:
            BridgedSubscribe() noexcept
            {
                // The base registered already; a production form would decide before registering.
                if (topology_t<Data>::binds(this))
                    this->disconnect();
            }

        protected:
            ~BridgedSubscribe() = default;
        };

        /** Publisher base of a StaticTo type: an empty handle. */
        template<class Data>
        class StaticPublish
        {
        protected:
            ~StaticPublish() = default;
        };

        template<class Data, class Topology = topology_t<Data>>
        using SubscribeFor = std::conditional_t<!Topology::cHasStatic, sub0::Subscribe<Data>,
                             std::conditional_t<Topology::cHasDynamic, BridgedSubscribe<Data>, StaticSubscribe<Data>>>;
    }

    /** Base of a receiver of Data, whichever topology Data has.
     *  Declare `void receive(const Data&) noexcept` without `override`: it overrides the broker's virtual for a
     *  runtime topology and is an ordinary member for StaticTo.
     */
    template<class Data>
    using Subscribe = detail::SubscribeFor<Data>;

    /** Base of a publisher of Data, whichever topology Data has. */
    template<class Data>
    using Publish = std::conditional_t<topology_t<Data>::cHasDynamic, sub0::Publish<Data>, detail::StaticPublish<Data>>;

    /** Base of a receiver of several message types, each with its own topology. */
    template<class... Datas>
    class SUB0PUB_SPIKE_EMPTY_BASES SubscribeAll : public Subscribe<Datas>...
    {
    };

    namespace detail
    {
        /** The topology each translation unit published Data with; shared by all of them, whatever they resolved. */
        template<class Data>
        struct TopologyRegistry
        {
            inline static std::atomic<uint32_t> fingerprint{0};
        };

        /** Debug-build report of a message type published with different topologies in different translation units.
         *  A unit that misses a StaticTo<> publishes to a broker nobody subscribes to: the message is lost without
         *  any other symptom, and the existing configuration check does not see it (no broker is involved on the
         *  static side). Compiled out unless SUB0PUB_CHECK_CONFIG.
         * @tparam Topology  The topology this translation unit resolved. A template parameter, so that two units
         *                   which disagree instantiate two functions and the linker cannot merge them into one.
         */
        template<class Data, class Topology>
        void checkTopology() noexcept
        {
#if SUB0PUB_SPIKE_AUDIT
            AuditLedger<Data>::topology(utility::typeHash<Topology>() | 1U);
#elif SUB0PUB_CHECK_CONFIG
            constexpr uint32_t mine = utility::typeHash<Topology>() | 1U; // never 0, which marks "unseen"
            uint32_t seen = 0;
            if (!TopologyRegistry<Data>::fingerprint.compare_exchange_strong(seen, mine, std::memory_order_relaxed) && seen != mine)
                SUB0PUB_CONFIG_MISMATCH("sub0pub: a message type was published with different topologies in different translation units");
#endif
        }
    }

    namespace detail
    {
        /** The compile-time checks every publication to bound receivers makes; none leaves code behind. */
        template<class Data, class Topology>
        constexpr void requireStatic() noexcept
        {
            Topology::requireComplete();
            Topology::template requireHandlers<Subscribe<Data>, Data>();
            Topology::template requireFilterOptIn<Data, config_t<Data>::filter>();
        }
    }

#if SUB0PUB_SPIKE_AUDIT
    namespace detail
    {
        /** Audit build: records one publication of Data by a publisher of type From and how far it reaches. */
        template<class From, class Data, class Topology>
        void auditPublication() noexcept
        {
            uint32_t reached = 0;
            if constexpr (Topology::cHasStatic)
            {
                Topology::template auditDeliveries<Data, Subscribe<Data>>();
                reached += Topology::template cReceivers<Data>;
            }
            if constexpr (Topology::cHasDynamic)
            {
                if constexpr (requires { sub0::detail::BrokerFor<Data>::receivers(); })
                    reached += sub0::detail::BrokerFor<Data>::receivers();
                else
                    reached = AuditLedger<Data>::cUnknown; // a broker the audit cannot ask
            }
            AuditLedger<Data>::published(&typeid(From), reached, allowsNoReceivers<config_t<Data>>);
        }
    }
#endif

    /** Publishes data along its type's topology: direct calls to bound receivers, the runtime broker, or both.
     * @param from  A publisher deriving from Publish<Data>.
     * @param data  The message; receivers see it by const reference for the duration of the call.
     */
    template<class From, class Data>
    SUB0PUB_FORCE_INLINE void publish(From& from, const Data& data) noexcept
    {
        using Topology = topology_t<Data>;
        [[maybe_unused]] const Publish<Data>& publisher = from; // From must publish Data in every topology
        detail::checkTopology<Data, Topology>();
#if SUB0PUB_SPIKE_AUDIT
        detail::auditPublication<From, Data, Topology>();
        const detail::AuditAccounted<Data> accounted;
#endif
        if constexpr (Topology::cHasStatic)
        {
            detail::requireStatic<Data, Topology>();
            // A publication that can reach nobody is a failure unless the type says otherwise. With only static
            // receivers that is known here, so it costs nothing at run time.
            static_assert(Topology::cHasDynamic || detail::allowsNoReceivers<config_t<Data>> || Topology::template cReceivers<Data> > 0,
                          "sub0pub: no receiver listed in this type's StaticTo<> can receive it, so the publication would "
                          "reach nobody; list a receiver, or configure the type with AllowNoReceivers");
            Topology::publish(data);
        }
        if constexpr (Topology::cHasDynamic)
            sub0::publish(from, data);
    }

    /** The number of receivers a publication of Data reaches at this moment.
     *  A constant for a StaticTo type. For a brokered type it reads the subscription table, which the library
     *  broker does not expose yet: in this spike it needs a broker with receivers() (sub0pub_spike/no_receivers.hpp).
     * @return Bound receivers that receive Data, plus runtime subscribers of Data.
     */
    template<class Data>
    [[nodiscard]] constexpr uint32_t receiverCount() noexcept
    {
        using Topology = topology_t<Data>;
        uint32_t count = 0;
        if constexpr (Topology::cHasStatic)
            count += Topology::template cReceivers<Data>;
        if constexpr (Topology::cHasDynamic)
        {
            static_assert(requires { sub0::detail::BrokerFor<Data>::receivers(); },
                          "sub0pub spike: receiverCount() of a brokered type needs a broker that exposes receivers()");
            count += sub0::detail::BrokerFor<Data>::receivers();
        }
        return count;
    }

    /** Publishes data and returns how many receivers it reached, without reporting when that is none.
     *  For a call site that treats "nobody is listening" as its own decision: it checks the result and warns,
     *  retries or ignores, whatever the type's default would have done.
     * @param from  A publisher deriving from Publish<Data>.
     * @param data  The message.
     * @return The number of receivers called.
     */
    template<class From, class Data>
    [[nodiscard]] SUB0PUB_FORCE_INLINE uint32_t tryPublish(From& from, const Data& data) noexcept
    {
        using Topology = topology_t<Data>;
        [[maybe_unused]] const Publish<Data>& publisher = from;
        detail::checkTopology<Data, Topology>();
#if SUB0PUB_SPIKE_AUDIT
        detail::auditPublication<From, Data, Topology>();
        const detail::AuditAccounted<Data> accounted;
#endif
        uint32_t reached = 0;
        if constexpr (Topology::cHasStatic)
        {
            detail::requireStatic<Data, Topology>();
            Topology::publish(data);
            reached += Topology::template cReceivers<Data>;
        }
        if constexpr (Topology::cHasDynamic)
        {
            static_assert(requires(const sub0::detail::BrokerFor<Data>& broker) { broker.publishCounted(data); },
                          "sub0pub spike: tryPublish() of a brokered type needs a broker with publishCounted()");
            reached += sub0::detail::BrokerFor<Data>().publishCounted(data);
        }
        return reached;
    }
} // namespace sub0::spike
