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
#include <cstdint>
#include <type_traits>

#include "sub0pub/broker.hpp"
#include "sub0pub/wiring.hpp"

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

    namespace detail
    {
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
#if SUB0PUB_SPIKE_CHECK_BOUND
            StaticSubscribe() noexcept
            {
                assert(topology_t<Data>::binds(this) && "sub0pub: this subscriber is not listed in its type's StaticTo<>");
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
#if SUB0PUB_CHECK_CONFIG
            constexpr uint32_t mine = utility::typeHash<Topology>() | 1U; // never 0, which marks "unseen"
            uint32_t seen = 0;
            if (!TopologyRegistry<Data>::fingerprint.compare_exchange_strong(seen, mine, std::memory_order_relaxed) && seen != mine)
                SUB0PUB_CONFIG_MISMATCH("sub0pub: a message type was published with different topologies in different translation units");
#endif
        }
    }

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
        if constexpr (Topology::cHasStatic)
        {
            Topology::requireComplete();
            Topology::template requireHandlers<Subscribe<Data>, Data>();
            Topology::publish(data);
        }
        if constexpr (Topology::cHasDynamic)
            sub0::publish(from, data);
    }
} // namespace sub0::spike
