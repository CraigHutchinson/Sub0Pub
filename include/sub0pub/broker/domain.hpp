/** Sub0Pub: Domain<Data>: session scope with independent tables for Scoped types
 * @remark Part of Sub0Pub (https://github.com/BareCpper/Sub0Pub), MIT License: see LICENSE.md.
 *         Included by the umbrella header <sub0pub/sub0pub.hpp>.
 */
#ifndef CROG_SUB0PUB_BROKER_DOMAIN_HPP
#define CROG_SUB0PUB_BROKER_DOMAIN_HPP

#include "sub0pub/broker/broker_impl.hpp"
#include <atomic>
#include <type_traits>

namespace sub0
{
    /** Session scope for Scoped types: independent subscription tables for the same Data type
     * @remark Lifetime contract: a Domain must outlive every Subscribe/Publish/Route bound to it (debug-checked).
     *         close() ends the session early: subscribe returns Closed, publish is dropped, current subscribers are
     *         detached, and in-flight dispatches are waited for.
     */
    template<class Data>
    class Domain
    {
        using Config = config_t<Data>;
        using BrokerT = detail::BrokerFor<Data>;
        static_assert(Config::storage == Storage::Scoped, "sub0pub: Domain<Data> requires a Data type configured with sub0::Scoped");
        static_assert(std::is_same_v<BrokerT, detail::BrokerImpl<Data, Config>>, "sub0pub: Scoped storage requires the library broker");
    public:
        Domain() = default;
        Domain(const Domain&) = delete;
        Domain& operator=(const Domain&) = delete;

        ~Domain()
        {
            close();
            if (table_.handles.load(std::memory_order_acquire) != 0)
                SUB0PUB_DOMAIN_LIFETIME("sub0pub: Domain destroyed while Subscribe/Publish/Route handles are still bound to it");
        }

        void close() noexcept { BrokerT::close(table_); }

        bool isClosed() const noexcept
        {
            detail::LockGuard<Config> lk(const_cast<typename BrokerT::TableT&>(table_));
            return table_.closed;
        }

    private:
        template<class, bool> friend class Subscribe;
        template<class, bool> friend class Publish;
        typename BrokerT::TableT table_;
    };
} // END: sub0

#endif // CROG_SUB0PUB_BROKER_DOMAIN_HPP
