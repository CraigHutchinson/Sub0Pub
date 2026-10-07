/** Sub0Pub: Subscribe<Data>: the runtime subscriber base, and the deliveries that need it complete
 * @remark Part of Sub0Pub (https://github.com/BareCpper/Sub0Pub), MIT License: see LICENSE.md.
 *         Included by the umbrella header <sub0pub/sub0pub.hpp>.
 */
#ifndef CROG_SUB0PUB_BROKER_SUBSCRIBE_HPP
#define CROG_SUB0PUB_BROKER_SUBSCRIBE_HPP

#include "sub0pub/broker/domain.hpp"
#include "sub0pub/utility/type_info.hpp"
#include "sub0pub/utility/streams.hpp"
#include <atomic>
#include <cstdint>
#include <type_traits>

namespace sub0
{
    /** Base type for an object that subscribes to some strong-typed Data
     * @tparam  Data  Type that will be received from publishers of corresponding type
     *
     * Interface: `void receive(const Data&) noexcept` (pure virtual), and `bool filter(const Data&) noexcept`
     * unless the type is configured with NoFilter. Declare receive() without `override` in a receiver that should
     * also work when its type is wired statically (StaticTo): the base then has no virtual function to override.
     * A receive() that does not match is still rejected: the class stays abstract here, and a StaticTo list
     * requires each of the type's subscribers it names to accept the type.
     *
     * Topology: this is the form for a type the runtime broker delivers. For a StaticTo type Subscribe<Data> is an
     * empty base instead (the specialisation below). For a StaticFirst type a subscriber the list names is called
     * directly and is not registered; any other subscriber registers as usual.
     *
     * Activation contract: single-threaded configurations register in the constructor. Concurrent configurations
     * (a Lock, e.g. SUB0PUB_THREAD_SAFE) do not: another thread could otherwise dispatch into the object before the
     * derived class is constructed. Call trySubscribe() at the end of the most-derived constructor (Route does this).
     *
     * Teardown contract: after disconnect() returns, receive() is not called again, on any thread. The destructor
     * disconnects too, but by then the derived object is already destroyed: when other threads may publish, call
     * disconnect() from the most-derived destructor (Route does this). Same-thread disconnect during a dispatch,
     * including from the subscriber's own receive(), is safe with Snapshot dispatch.
     *
     * @remark The destructor is protected and non-virtual: a subscriber is destroyed as its own type, never through a
     *         Subscribe<Data>* (no vtable destructor slots, no operator delete dependency on small targets).
     */
    template<class Data, bool Wired>
    class Subscribe : public detail::SubscriberInterface<Data, config_t<Data>::filter>
    {
        using Config = config_t<Data>;
        using Broker = detail::BrokerFor<Data>;
        template<class> friend class Domain;
        template<class, class> friend class detail::BrokerImpl;
    public:
        /** Registers the subscriber (single-threaded configurations)
         * @param[in] typeId, typeName  Optional unique identity of Data for inter-process streams (SUB0PUB_TYPEIDNAME)
         */
        template<class C = Config, std::enable_if_t<C::storage == Storage::Global, int> = 0>
        Subscribe(
#if SUB0PUB_TYPEIDNAME
            const uint32_t typeId = 0, const char* typeName = nullptr
#endif
        ) noexcept
        {
#if SUB0PUB_TYPEIDNAME
            detail::TypeInfo<Data>::set(typeId, typeName);
#endif
            activateIfSingleThreaded();
        }

        /** Registers the subscriber in `domain` (Scoped types; single-threaded configurations) */
        template<class C = Config, std::enable_if_t<C::storage == Storage::Scoped, int> = 0>
        explicit Subscribe(Domain<Data>& domain) noexcept : broker_(domain.table_) { activateIfSingleThreaded(); }

        Subscribe(const Subscribe&) = delete;
        Subscribe& operator=(const Subscribe&) = delete;

        /** @return Whether this subscriber is registered and will receive published Data
         * @remark False if the table was full (or the Domain closed) at registration, after disconnect(), or, for
         *         concurrent configurations, before trySubscribe(). Always true for a subscriber that its type's
         *         StaticFirst list names: it receives by a direct call, without a registration to lose.
         */
        bool isSubscribed() const noexcept { return subscribed_.load() || isBound(); }

        /** Register, or retry registration after SubscribeResult::CapacityExceeded
         * @return SubscribeResult::Subscribed if now (or already) registered; CapacityExceeded with the table
         *         unchanged; Closed if the Domain has been closed
         */
        SubscribeResult trySubscribe() noexcept
        {
            if (isSubscribed())
                return SubscribeResult::Subscribed;
            const SubscribeResult result = broker_.trySubscribe(this);
            // The library broker records the registration under its table lock; an application broker cannot
            if constexpr (!std::is_same_v<Broker, detail::BrokerImpl<Data, Config>>)
                subscribed_.store(result == SubscribeResult::Subscribed);
            return result;
        }

        /// Stop receiving. Idempotent; safe from within receive(); see the teardown contract above
        void disconnect() noexcept
        {
            const bool wasSubscribed = subscribed_.exchange(false);
            // Concurrent: always, so a subscriber already detached by Domain::close() still waits out a callback in
            // progress on another thread. Single-threaded: close() already made it safe; nothing left to do.
            if (detail::cConcurrent<Config> || wasSubscribed)
                broker_.disconnect(this);
        }

        /** Stop delivery of the current publication to the remaining subscribers
         * @note Only meaningful from within receive() or filter()
         */
        void cancel() const noexcept
        {
            static_assert(Config::context != Context::None,
                          "sub0pub: cancel() needs a publish context: define SUB0PUB_CANCEL, or configure the type with "
                          "sub0::ThreadLocalContext or sub0::StaticContext");
            broker_.cancel();
        }

#if SUB0PUB_TYPEIDNAME
        /** @return Null-terminated name given to Data, or nullptr */
        const char* typeName() const noexcept { return detail::TypeInfo<Data>::typeName(); }

        /** Stream operator for diagnostics reporting */
        friend OStream& operator<< (OStream& stream, const Subscribe<Data>& subscriber)
        { return stream << subscriber.typeName() << '{' << (const void*)&subscriber << '}'; }
#endif

    protected:
        ~Subscribe() { disconnect(); }

        void activateIfSingleThreaded() noexcept
        {
            if constexpr (!detail::cConcurrent<Config>)
                trySubscribe();
        }

        /// For bindings (Route): publish into this subscriber's table with an ingress origin
        void injectFrom(const void* origin, const Data& data) const noexcept { broker_.publish(data, origin, nullptr); }

    private:
        /// Whether this subscriber is one of the receivers its type's StaticFirst list calls directly
        bool isBound() const noexcept
        {
            if constexpr (detail::cBridged<Data>)
                return detail::topology_t<Data>::binds(this);
            else
                return false;
        }

        Broker broker_;
        detail::Flag<detail::cConcurrent<Config>> subscribed_;
    };

    /** Subscribe<Data> for a statically wired type (StaticTo): an empty base
     * @tparam  Data  Type whose configuration names its receivers
     *
     * The receiver is a plain class: no vtable, no registration and no state come from this base, and its
     * `receive(const Data&)` is an ordinary member that the type's StaticTo list calls directly. What remains is
     * what makes the same source valid for either topology: the base class name, and the checks that a receiver
     * was not forgotten.
     *
     * @remark The members that need a subscription table (isSubscribed(), trySubscribe(), disconnect(), cancel())
     *         do not exist: using one is a compile error that says what a statically wired type cannot do.
     * @remark A subscriber the list does not name is never called; SUB0PUB_UNLISTED_CHECK reports its construction.
     */
    template<class Data>
    class Subscribe<Data, true>
    {
    public:
#if SUB0PUB_TYPEIDNAME || SUB0PUB_UNLISTED_CHECK || SUB0PUB_CHECK_CONFIG
        /** @param[in] typeId, typeName  Optional unique identity of Data for inter-process streams (SUB0PUB_TYPEIDNAME) */
        Subscribe(
#if SUB0PUB_TYPEIDNAME
            const uint32_t typeId = 0, const char* typeName = nullptr
#endif
        ) noexcept
        {
#if SUB0PUB_TYPEIDNAME
            detail::TypeInfo<Data>::set(typeId, typeName);
#endif
            detail::checkConfig<Data, config_t<Data>>();
#if SUB0PUB_UNLISTED_CHECK
            // An incomplete-type error here: include sub0pub/sub0pub.hpp (or sub0pub/wiring/static_topology.hpp)
            if (!detail::topology_t<Data>::binds(this))
                SUB0PUB_UNLISTED_RECEIVER("sub0pub: a subscriber of a statically wired Data type is not in the type's StaticTo list, so it is never called");
#endif
        }
#else
        Subscribe() = default; // trivial: a receiver is as cheap to construct as a class without this base
#endif

        Subscribe(const Subscribe&) = delete;
        Subscribe& operator=(const Subscribe&) = delete;

#if SUB0PUB_TYPEIDNAME
        /** @return Null-terminated name given to Data, or nullptr */
        const char* typeName() const noexcept { return detail::TypeInfo<Data>::typeName(); }
#endif

    protected:
        ~Subscribe() = default;
    };

    namespace kit
    {
        template<class Data>
        void deliver(Subscribe<Data>* s, const Data& data) noexcept
        {
            if constexpr (config_t<Data>::filter)
                if (!s->filter(data))
                    return;
            s->receive(data);
        }

        namespace slot
        {
            template<class Data> Subscribe<Data>* load(Subscribe<Data>* const& p) noexcept { return p; }
            template<class Data> Subscribe<Data>* load(const std::atomic<Subscribe<Data>*>& p) noexcept { return p.load(std::memory_order_seq_cst); }
        }

        template<class Data, bool MayBeCleared, class Slot>
        void deliverAt(Slot& slot, const Data& data) noexcept
        {
            Subscribe<Data>* const s = slot::load<Data>(slot);
            if constexpr (MayBeCleared)
                if (s == nullptr)
                    return;
            if constexpr (config_t<Data>::filter)
            {
                if (!s->filter(data))
                    return;
                if (slot::load<Data>(slot) != s) // disconnected (or destroyed) inside its own filter()
                    return;
            }
            s->receive(data);
        }
    }

    template<class Data, class Config>
    template<bool S, std::enable_if_t<S, int>>
    void detail::BrokerImpl<Data, Config>::close(TableT& t) noexcept
    {
        uint32_t n;
        Subscribe<Data>* detached[Config::capacity];
        {
            LockGuard<Config> lk(t);
            UseScope<TableT, cThreadCheck<Config>> use(t);
            checkNotDispatching(t); // closing detaches every subscriber: a table change, like unsubscribing
            t.closed = true;
            n = t.count;
            for (uint32_t i = 0; i < n; ++i)
            {
                if constexpr (!cConcurrent<Config>)
                    detached[i] = t.entries[i];
                t.entries[i]->subscribed_.store(false);
                t.entries[i] = nullptr; // a Direct dispatch in progress re-reads its slot after filter(): not called
            }
            t.count = 0;
            if constexpr (cConcurrent<Config>)
                forgetInActiveDispatches(t, nullptr);
        }
        if constexpr (cConcurrent<Config>)
            waitWhileCalledElsewhere(t, nullptr);
        else
            for (uint32_t i = 0; i < n; ++i)
                kit::forgetInOwnDispatches<Data>(&t, detached[i]);
    }
} // END: sub0

#endif // CROG_SUB0PUB_BROKER_SUBSCRIBE_HPP
