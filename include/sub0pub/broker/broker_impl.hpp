/** Sub0Pub: The library broker (BrokerImpl) and the subscriber interface it dispatches to
 * @remark Part of Sub0Pub (https://github.com/BareCpper/Sub0Pub), MIT License: see LICENSE.md.
 *         Included by the umbrella header <sub0pub/sub0pub.hpp>.
 */
#ifndef CROG_SUB0PUB_BROKER_BROKER_IMPL_HPP
#define CROG_SUB0PUB_BROKER_BROKER_IMPL_HPP

#include "sub0pub/broker/kit.hpp"
#include "sub0pub/broker/config_check.hpp"
#include <cstring>
#include <atomic>
#include <cstdint>
#include <thread>
#include <type_traits>

namespace sub0
{
    namespace detail
    {
        /// Subscriber interface, with filter() only when the configuration asks for it
        template<class Data, bool Filter>
        class SubscriberInterface
        {
        public:
            /** Receive published Data */
            virtual void receive(const Data& data) noexcept = 0;

            /** filter() is not enabled for this Data type: a subscriber that declares `bool filter(const Data&)` fails
             *  to compile here (conflicting return type / overrides a final function), with or without `override`,
             *  rather than being silently ignored. Enable it with SUB0PUB_FILTER or sub0::Filter. Never called.
             */
            virtual void filter(const Data&) noexcept final {}
        protected:
            ~SubscriberInterface() = default;
        };
        template<class Data>
        class SubscriberInterface<Data, true>
        {
        public:
            /** Receive published Data */
            virtual void receive(const Data& data) noexcept = 0;
            /** @return false to skip receive() for this message */
            virtual bool filter(const Data&) noexcept { return true; }
        protected:
            ~SubscriberInterface() = default;
        };

        /** The library broker for one Data type with one resolved configuration (the default Implementation)
         *
         * Broker concept (what Subscribe/Publish/Route require of any Implementation<>):
         *   Broker() noexcept                                            Global storage
         *   SubscribeResult trySubscribe(Subscribe<Data>*) noexcept
         *   void disconnect(Subscribe<Data>*) noexcept                   after return: no further receive() calls
         *   void publish(const Data&, const void* origin, PublishReport*) const noexcept
         *   void cancel() const noexcept
         * Deliver with kit::deliverAt() inside a kit::DispatchScope.
         */
        template<class Data, class Config>
        class BrokerImpl
        {
            static_assert(Config::capacity > 0, "sub0pub: Capacity must be at least 1");
            static_assert(!(Config::dispatch == Dispatch::Snapshot && Config::context == Context::None),
                          "sub0pub: Snapshot needs a publish context (StaticContext or ThreadLocalContext): a subscriber "
                          "disconnected during a dispatch is removed from that dispatch's snapshot through its frame");
            static_assert(!cConcurrent<Config> || Config::dispatch == Dispatch::Snapshot,
                          "sub0pub: a Lock requires Snapshot dispatch (receivers are called outside the lock)");
            static_assert(!cConcurrent<Config> || Config::context == Context::ThreadLocal,
                          "sub0pub: a Lock requires ThreadLocalContext (disconnect must not wait on its own dispatch, and a "
                          "StaticContext frame stack shared by concurrent publishers lets one thread's cancel() and "
                          "frames act on another thread's dispatch)");

            static constexpr bool cScoped = Config::storage == Storage::Scoped;

        public:
            using Configuration = Config;
            using TableT = Table<Data, Config>;
            static constexpr uint32_t cMaxSubscriptions = Config::capacity; ///< Subscription table size

            template<bool S = cScoped, std::enable_if_t<!S, int> = 0>
            BrokerImpl() noexcept { checkConfig<Data, Config>(); }

            template<bool S = cScoped, std::enable_if_t<S, int> = 0>
            explicit BrokerImpl(TableT& table) noexcept : scope_(table) { checkConfig<Data, Config>(); }

            SubscribeResult trySubscribe(Subscribe<Data>* subscriber) noexcept
            {
                TableT& t = table();
                LockGuard<Config> lk(t);
                UseScope<TableT, cThreadCheck<Config>> use(t);
                checkNotDispatching(t);
                if constexpr (cScoped)
                    if (t.closed)
                        return SubscribeResult::Closed;
                if (t.count >= Config::capacity)
                    return SubscribeResult::CapacityExceeded; // table unchanged: bounded, no out-of-bounds write
                t.entries[t.count++] = subscriber;
                subscriber->subscribed_.store(true); // under the table lock, so a concurrent Domain::close() wins
                return SubscribeResult::Subscribed;
            }

            /** Remove `subscriber`, keeping the order of the others; on return no dispatch (on any thread) calls it again
             * @remark Dispatches in progress forget it. Concurrent configurations then wait while another thread is
             *         inside its callback. Safe from within the subscriber's own receive().
             * @warning Concurrent: do not disconnect, from inside a receive(), a subscriber that another thread's
             *          receive() is disconnecting you from at the same time (mutual wait). Defer such teardown.
             */
            void disconnect(Subscribe<Data>* subscriber) noexcept
            {
                TableT& t = table();
                {
                    LockGuard<Config> lk(t);
                    UseScope<TableT, cThreadCheck<Config>> use(t);
                    // A plain search and shift: fewer instructions than std::find/std::move for a table this small
                    for (uint32_t i = 0; i < t.count; ++i)
                        if (t.entries[i] == subscriber)
                        {
                            checkNotDispatching(t);
                            for (uint32_t j = i + 1; j < t.count; ++j)
                                t.entries[j - 1] = t.entries[j];
                            --t.count;
                            break;
                        }
                    if constexpr (cConcurrent<Config>)
                        forgetInActiveDispatches(t, subscriber);
                }
                if constexpr (cConcurrent<Config>)
                    waitWhileCalledElsewhere(t, subscriber);
                else
                    kit::forgetInOwnDispatches<Data>(&t, subscriber);
            }

            void publish(const Data& data, const void* origin = nullptr, PublishReport* report = nullptr) const noexcept
            {
                TableT& t = table();
                if constexpr (cConcurrent<Config>)
                {
                    std::atomic<Subscribe<Data>*> snapshot[Config::capacity];
                    ActiveDispatch<Data> active{snapshot, 0, {nullptr}, std::this_thread::get_id()};
                    {
                        LockGuard<Config> lk(t);
                        if constexpr (cScoped)
                            if (t.closed)
                                return;
                        active.count = t.count;
                        for (uint32_t i = 0; i < active.count; ++i)
                            snapshot[i].store(t.entries[i], std::memory_order_relaxed);
                        t.link(active);
                    }
                    {
                        kit::DispatchScope<Data> scope(&t, origin, report, nullptr, 0);
                        for (uint32_t i = 0; !scope.canceled() && i < active.count; ++i)
                        {
                            Subscribe<Data>* const s = snapshot[i].load(std::memory_order_seq_cst);
                            if (s == nullptr)
                                continue;
                            active.current.store(s, std::memory_order_seq_cst);
                            kit::deliverAt<Data>(snapshot[i], data); // re-checks the slot: not disconnected meanwhile
                            active.current.store(nullptr, std::memory_order_seq_cst);
                        }
                    }
                    LockGuard<Config> lk(t);
                    t.unlink(active);
                }
                else if constexpr (Config::dispatch == Dispatch::Snapshot)
                {
                    UseScope<TableT, cThreadCheck<Config>> use(t);
                    Subscribe<Data>* snapshot[Config::capacity];
                    if constexpr (cScoped)
                        if (t.closed)
                            return;
                    const uint32_t count = t.count;
                    // Pointer arrays are trivially copyable and do not overlap.
                    std::memcpy(snapshot, t.entries, count * sizeof(snapshot[0]));
                    kit::DispatchScope<Data> scope(&t, origin, report, snapshot, count);
                    for (uint32_t i = 0; !scope.canceled() && i < count; ++i)
                        kit::deliverAt<Data>(snapshot[i], data);
                }
                else
                {
                    // Direct: a nested publish is fine; only changing this table during its own dispatch is not
                    UseScope<TableT, cThreadCheck<Config>> use(t);
                    kit::DispatchScope<Data> scope(&t, origin, report, nullptr, 0);
                    for (uint32_t i = 0; !scope.canceled() && i < t.count; ++i)
                        kit::deliverAt<Data, false>(t.entries[i], data);
                }
            }

            /// No-op without a publish context: Subscribe/Publish::cancel() reject that at compile time
            void cancel() const noexcept
            {
                if constexpr (Config::context != Context::None)
                    kit::cancel<Data>(&table());
            }

            /// Close a Scoped table: reject subscriptions, drop publishes, detach subscribers, then quiesce
            /// (a member template, so explicitly instantiating a Global broker does not instantiate it)
            template<bool S = cScoped, std::enable_if_t<S, int> = 0>
            static void close(TableT& t) noexcept;

        private:
            // The concurrent helpers are member templates, so explicitly instantiating a single-threaded broker
            // (e.g. to export it from a module) does not instantiate them

            /// Concurrent: null `s` (or every entry when s == nullptr) in all active snapshots. Call under the table lock.
            template<bool C = cConcurrent<Config>, std::enable_if_t<C, int> = 0>
            static void forgetInActiveDispatches(TableT& t, const Subscribe<Data>* s) noexcept
            {
                for (ActiveDispatch<Data>* a = t.activeHead; a; a = a->next)
                    for (uint32_t i = 0; i < a->count; ++i)
                        if (s == nullptr || a->snapshot[i].load(std::memory_order_relaxed) == s)
                            a->snapshot[i].store(nullptr, std::memory_order_seq_cst);
            }

            /// Concurrent: wait while another thread is inside `s`'s callback (any callback when s == nullptr)
            template<bool C = cConcurrent<Config>, std::enable_if_t<C, int> = 0>
            static void waitWhileCalledElsewhere(TableT& t, const Subscribe<Data>* s) noexcept
            {
                const std::thread::id me = std::this_thread::get_id();
                for (;;)
                {
                    bool busy = false;
                    {
                        LockGuard<Config> lk(t);
                        for (ActiveDispatch<Data>* a = t.activeHead; a && !busy; a = a->next)
                        {
                            Subscribe<Data>* const current = a->current.load(std::memory_order_seq_cst);
                            busy = a->thread != me && current != nullptr && (s == nullptr || current == s);
                        }
                    }
                    if (!busy)
                        return;
                    yieldThread<Config>();
                }
            }

            /// DirectChecked: only a use of the table currently being iterated on this thread is a violation
            static void checkNotDispatching(TableT& t) noexcept
            {
                if constexpr (Config::dispatch == Dispatch::DirectChecked)
                    if (kit::ownDispatches<Data>(&t) != 0)
                        SUB0PUB_REENTRANT_VIOLATION("sub0pub: subscribing or unsubscribing a Data type during its own dispatch requires Snapshot dispatch (SUB0PUB_REENTRANT_SAFE or sub0::Snapshot)");
                (void)t;
            }

            TableT& table() const noexcept { return *scope_.get(global_); }

            ScopeRef<TableT, cScoped> scope_;
            inline static TableT global_;
        };

        /// The broker implementation a Data type's configuration selects
        template<class Data>
        using BrokerFor = typename config_t<Data>::template broker<Data, config_t<Data>>;

        /// The broker every use of Data goes through
        template<class Data>
        using Broker = BrokerFor<Data>;
    } // END: detail
} // END: sub0

#endif // CROG_SUB0PUB_BROKER_BROKER_IMPL_HPP
