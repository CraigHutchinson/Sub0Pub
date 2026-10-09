/** Sub0Pub: Subscription table and dispatch bookkeeping: locking, active dispatches, scopes, frames
 * @remark Part of Sub0Pub (https://github.com/BareCpper/Sub0Pub), MIT License: see LICENSE.md.
 *         Included by the umbrella header <sub0pub/sub0pub.hpp>.
 */
#ifndef CROG_SUB0PUB_BROKER_TABLE_HPP
#define CROG_SUB0PUB_BROKER_TABLE_HPP

#include "sub0pub/config.hpp"
#include "sub0pub/broker/results.hpp"
#include <atomic>
#include <cstdint>
#include <thread>
#include <type_traits>

namespace sub0
{
    namespace detail
    {
        template<class Config>
        struct LockGuard
        {
            explicit LockGuard(typename Config::Lock& l) noexcept : l_(l) { l_.lock(); }
            ~LockGuard() { l_.unlock(); }
            LockGuard(const LockGuard&) = delete;
            LockGuard& operator=(const LockGuard&) = delete;
            typename Config::Lock& l_;
        };

        /// Whether a configuration is for concurrent use (has a Lock)
        template<class Config>
        constexpr bool cConcurrent = !std::is_same_v<typename Config::Lock, NoLock>;

        /** One dispatch in progress (concurrent configurations only), linked into its table under the table lock
         * @remark unsubscribe()/close() null a removed subscriber out of every active snapshot, then wait only while
         *         another thread is inside that subscriber's callback (`current`): bounded by one callback, no starvation.
         *         Dispatcher: store current, re-load entry; writer: store null entry, load current. Both seq_cst, so at
         *         least one side observes the other: a subscriber is never called after unsubscribe() returns.
         */
        template<class Data>
        struct ActiveDispatch
        {
            std::atomic<Subscribe<Data>*>* snapshot;
            uint32_t count;
            std::atomic<Subscribe<Data>*> current{nullptr};
            std::thread::id thread;
            ActiveDispatch* next = nullptr;
            ActiveDispatch* previous = nullptr;
        };

        template<class Data, bool Concurrent>
        struct ActiveList {};
        template<class Data>
        struct ActiveList<Data, true>
        {
            ActiveDispatch<Data>* activeHead = nullptr;

            void link(ActiveDispatch<Data>& d) noexcept
            {
                d.next = activeHead;
                if (activeHead)
                    activeHead->previous = &d;
                activeHead = &d;
            }
            void unlink(ActiveDispatch<Data>& d) noexcept
            {
                (d.previous ? d.previous->next : activeHead) = d.next;
                if (d.next)
                    d.next->previous = d.previous;
            }
        };

        /// Lifecycle state, only for Scoped storage: closed flag and count of bound handles
        template<bool Scoped>
        struct ScopeState {};
        template<>
        struct ScopeState<true>
        {
            bool closed = false;
            std::atomic<uint32_t> handles{0};
        };

        /// A subscriber's subscribed flag: atomic only where other threads may read it (a Lock)
        template<bool Atomic>
        struct Flag
        {
            bool load() const noexcept { return v_; }
            void store(bool b) noexcept { v_ = b; }
            bool exchange(bool b) noexcept { const bool old = v_; v_ = b; return old; }
            bool v_ = false;
        };
        template<>
        struct Flag<true>
        {
            bool load() const noexcept { return v_.load(std::memory_order_relaxed); }
            void store(bool b) noexcept { v_.store(b, std::memory_order_relaxed); }
            bool exchange(bool b) noexcept { return v_.exchange(b, std::memory_order_relaxed); }
            std::atomic<bool> v_{false};
        };

        /// Whether a configuration gets the debug check for unlocked concurrent use (SUB0PUB_THREAD_CHECK)
        template<class Config>
        constexpr bool cThreadCheck = SUB0PUB_THREAD_CHECK && !cConcurrent<Config>;

        /// A token unique to the calling thread (the address of a thread_local)
        inline std::uintptr_t threadToken() noexcept
        {
            thread_local const char anchor = 0;
            return reinterpret_cast<std::uintptr_t>(&anchor);
        }

        /// Table state for the unlocked-use check: which thread is using the table, 0 when none
        template<bool Enabled>
        struct ThreadCheck {};
        template<>
        struct ThreadCheck<true>
        {
            std::atomic<std::uintptr_t> user{0};
        };

        /** Marks one publish/subscribe/unsubscribe of an unlocked table; reports an overlap with another thread's
         *  (nested use on the same thread is fine). Empty unless SUB0PUB_THREAD_CHECK.
         */
        template<class TableT, bool Enabled>
        struct UseScope
        {
            explicit UseScope(TableT&) noexcept {}
        };
        template<class TableT>
        struct UseScope<TableT, true>
        {
            explicit UseScope(TableT& t) noexcept : t_(t)
            {
                const std::uintptr_t me = threadToken();
                std::uintptr_t seen = 0;
                acquired_ = t_.user.compare_exchange_strong(seen, me, std::memory_order_acquire, std::memory_order_relaxed);
                if (!acquired_ && seen != me)
                    SUB0PUB_THREAD_VIOLATION("sub0pub: a Data type was used from two threads at once without a lock: "
                                             "configure the type with sub0::LockWith<L>");
            }
            ~UseScope()
            {
                if (acquired_)
                    t_.user.store(0, std::memory_order_release);
            }
            UseScope(const UseScope&) = delete;
            UseScope& operator=(const UseScope&) = delete;
            TableT& t_;
            bool acquired_;
        };

        /// Subscription table for one Data type (Global) or one Domain (Scoped). Empty bases cost nothing.
        template<class Data, class Config>
        struct Table : Config::Lock, ActiveList<Data, cConcurrent<Config>>, ScopeState<Config::storage == Storage::Scoped>,
                      ThreadCheck<cThreadCheck<Config>>
        {
            uint32_t count = 0;
            Subscribe<Data>* entries[Config::capacity] = {};
        };

        /** One dispatch in progress on this thread. Frames form a per-thread stack (per Data type).
         * @remark `table` identifies the subscription table being dispatched, so cancel(), re-entrancy checks and
         *         unsubscribe act only on their own table (per Domain for Scoped storage). `origin` is the ingress
         *         binding that injected the message (split horizon); `report` collects route results (opt-in).
         */
        template<class Data>
        struct Frame
        {
            const void* table;
            const void* origin;
            PublishReport* report;
            Subscribe<Data>** snapshot; ///< this dispatch's snapshot (Snapshot dispatch), else nullptr
            uint32_t count;             ///< entries in snapshot
            bool canceled;
            Frame* previous;
        };

        /** Where a type's dispatch frames live: its configured context, or, for DirectChecked without one, a
         *  thread_local context that only the re-entrancy check uses (cancel() still needs a configured context)
         */
        template<class Config>
        constexpr Context frameContext = (Config::context == Context::None && Config::dispatch == Dispatch::DirectChecked)
                                         ? Context::ThreadLocal : Config::context;

        template<class Data, Context C>
        struct PublishContext
        {
            static constexpr bool enabled = false;
        };
        template<class Data>
        struct PublishContext<Data, Context::ThreadLocal>
        {
            static constexpr bool enabled = true;
            static Frame<Data>*& top() noexcept { return top_; }
            inline static thread_local Frame<Data>* top_ = nullptr;
        };
        template<class Data>
        struct PublishContext<Data, Context::Static>
        {
            static constexpr bool enabled = true;
            static Frame<Data>*& top() noexcept { return top_; }
            inline static Frame<Data>* top_ = nullptr;
        };

        template<class Lock, class = void> struct has_yield : std::false_type {};
        template<class Lock> struct has_yield<Lock, std::void_t<decltype(Lock::yield())>> : std::true_type {};

        /// Yield while quiescing: Lock::yield() if the lock type provides one (RTOS), else std::this_thread::yield()
        template<class Config>
        void yieldThread() noexcept
        {
            if constexpr (has_yield<typename Config::Lock>::value)
                Config::Lock::yield();
            else
                std::this_thread::yield();
        }

        /// Scope handle held by each Subscribe/Publish: empty for Global storage; Scoped counts bound handles
        template<class TableT, bool Scoped>
        struct ScopeRef
        {
            TableT* get(TableT& global) const noexcept { return &global; }
        };
        template<class TableT>
        struct ScopeRef<TableT, true>
        {
            explicit ScopeRef(TableT& t) noexcept : t_(&t) { t_->handles.fetch_add(1, std::memory_order_relaxed); }
            ~ScopeRef() { t_->handles.fetch_sub(1, std::memory_order_release); }
            ScopeRef(const ScopeRef&) = delete;
            ScopeRef& operator=(const ScopeRef&) = delete;
            TableT* get(TableT&) const noexcept { return t_; }
            TableT* t_;
        };
    } // END: detail
} // END: sub0

#endif // CROG_SUB0PUB_BROKER_TABLE_HPP
