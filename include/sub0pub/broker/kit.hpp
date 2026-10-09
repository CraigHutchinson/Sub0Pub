/** Sub0Pub: Broker author kit: dispatch frames and delivery for application-defined brokers (Implementation<>)
 * @remark Part of Sub0Pub (https://github.com/BareCpper/Sub0Pub), MIT License: see LICENSE.md.
 *         Included by the umbrella header <sub0pub/sub0pub.hpp>.
 */
#ifndef CROG_SUB0PUB_BROKER_KIT_HPP
#define CROG_SUB0PUB_BROKER_KIT_HPP

#include "sub0pub/broker/table.hpp"
#include <cstdint>
#include <type_traits>

namespace sub0
{
    /** Broker author kit: what an application-defined broker (Implementation<>) builds on */
    namespace kit
    {
        /** RAII dispatch frame: push while calling receivers so cancel(), routes (origin, report) and same-thread
         * unsubscribe work. Zero-size when the Data type's configuration has no publish context.
         */
        template<class Data>
        class DispatchScope
        {
            using Ctx = detail::PublishContext<Data, detail::frameContext<config_t<Data>>>;
        public:
            DispatchScope(const void* table, const void* origin, PublishReport* report,
                          Subscribe<Data>** snapshot, uint32_t count) noexcept
                : frame_(makeFrame(table, origin, report, snapshot, count))
            {
                if constexpr (Ctx::enabled)
                    Ctx::top() = &frame_;
            }
            ~DispatchScope()
            {
                if constexpr (Ctx::enabled)
                    Ctx::top() = frame_.previous;
            }
            DispatchScope(const DispatchScope&) = delete;
            DispatchScope& operator=(const DispatchScope&) = delete;

            bool canceled() const noexcept
            {
                if constexpr (Ctx::enabled)
                    return frame_.canceled;
                else
                    return false;
            }

        private:
            struct NoFrame {};
            using FrameT = std::conditional_t<Ctx::enabled, detail::Frame<Data>, NoFrame>;

            /// Initialise the frame once, in place (no zero-fill then overwrite: avoids memset on small targets)
            static FrameT makeFrame(const void* table, const void* origin, PublishReport* report,
                                    Subscribe<Data>** snapshot, uint32_t count) noexcept
            {
                if constexpr (Ctx::enabled)
                    return FrameT{ table, origin, report, snapshot, count, false, Ctx::top() };
                else
                {
                    (void)table; (void)origin; (void)report; (void)snapshot; (void)count;
                    return FrameT{};
                }
            }

            FrameT frame_;
        };

        /// Call one subscriber: filter() (when configured) then receive(). Prefer deliverAt() in a dispatch loop.
        template<class Data>
        void deliver(Subscribe<Data>* s, const Data& data) noexcept;

        /** Call the subscriber held in a dispatch-owned slot (a snapshot entry, or a table entry for Direct dispatch):
         * filter() when configured, then receive() only if the slot still holds that subscriber. A filter() that
         * unsubscribes (or destroys) its own subscriber clears the slot, so receive() is not called. Only the slot is
         * re-read, never the subscriber. Without a filter this is exactly one load and one call.
         * @tparam MayBeCleared  false for a live Direct table entry, which is never null: skips that check
         */
        template<class Data, bool MayBeCleared = true, class Slot>
        void deliverAt(Slot& slot, const Data& data) noexcept;

        /// Innermost dispatch in progress on this thread for Data, or nullptr
        template<class Data>
        const detail::Frame<Data>* activeDispatch() noexcept
        {
            using Ctx = detail::PublishContext<Data, detail::frameContext<config_t<Data>>>;
            if constexpr (Ctx::enabled)
                return Ctx::top();
            else
                return nullptr;
        }

        /// Number of dispatches of `table` in progress on this thread
        template<class Data>
        uint32_t ownDispatches(const void* table) noexcept
        {
            uint32_t n = 0;
            for (const detail::Frame<Data>* f = activeDispatch<Data>(); f; f = f->previous)
                n += (f->table == table) ? 1U : 0U;
            return n;
        }

        /// Cancel the innermost dispatch of `table` on this thread; no-op if that table is not being dispatched
        template<class Data>
        void cancel(const void* table) noexcept
        {
            using Ctx = detail::PublishContext<Data, detail::frameContext<config_t<Data>>>;
            static_assert(config_t<Data>::context != Context::None,
                          "sub0pub: cancel() needs a publish context: configure the type with "
                          "sub0::ThreadLocalContext or sub0::StaticContext");
            if constexpr (Ctx::enabled)
                for (detail::Frame<Data>* f = Ctx::top(); f; f = f->previous)
                    if (f->table == table)
                    {
                        f->canceled = true;
                        return;
                    }
        }

        /// Remove `s` from this thread's in-progress snapshots of `table`, so a subscriber unsubscribed (and possibly
        /// destroyed) during a dispatch on this thread is not called afterwards by that dispatch
        template<class Data>
        void forgetInOwnDispatches(const void* table, const Subscribe<Data>* s) noexcept
        {
            using Ctx = detail::PublishContext<Data, detail::frameContext<config_t<Data>>>;
            if constexpr (Ctx::enabled)
                for (detail::Frame<Data>* f = Ctx::top(); f; f = f->previous)
                    if (f->table == table && f->snapshot)
                        for (uint32_t i = 0; i < f->count; ++i)
                            if (f->snapshot[i] == s)
                                f->snapshot[i] = nullptr;
        }
    }
} // END: sub0

#endif // CROG_SUB0PUB_BROKER_KIT_HPP
