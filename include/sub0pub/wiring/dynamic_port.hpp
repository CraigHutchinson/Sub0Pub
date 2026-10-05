/** Sub0Pub: DynamicPort<T, N>: runtime subscribers behind a static wiring, with no policy
 * @remark Part of Sub0Pub (https://github.com/BareCpper/Sub0Pub), MIT License: see LICENSE.md.
 *         Included by the umbrella header <sub0pub/sub0pub.hpp>.
 */
#ifndef CROG_SUB0PUB_WIRING_DYNAMIC_PORT_HPP
#define CROG_SUB0PUB_WIRING_DYNAMIC_PORT_HPP

#include "sub0pub/config_macros.hpp"
#include <cstdint>

namespace sub0
{
    /** Runtime subscribers behind a static wiring: bind the port like any receiver; receivers come and go at runtime.
     *  A fixed slot array with no policy: no filter, no publish context, no locking. Delivery is in add() order.
     *  For policy on the dynamic side (capacity, filter, locking, domains, teardown contract) use BrokerPort<T>.
     *  @warning Not thread-safe: add(), remove() and publishing must not run concurrently.
     */
    template<class T, uint32_t N = 8>
    class DynamicPort
    {
    public:
        /// Implemented by runtime receivers
        struct Receiver
        {
            virtual void receive(const T&) noexcept = 0;
        protected:
            ~Receiver() = default;
        };

        void add(Receiver* r) noexcept { (void)tryAdd(r); }

        /// As add(), but reports whether it fit (capacity exceeded is otherwise silent)
        bool tryAdd(Receiver* r) noexcept
        {
            if (count_ >= N)
                return false;
            entries_[count_++] = r;
            return true;
        }

        /// Remove r, keeping the order of the others
        void remove(Receiver* r) noexcept
        {
            for (uint32_t i = 0; i < count_; ++i)
                if (entries_[i] == r)
                {
                    for (uint32_t j = i + 1; j < count_; ++j)
                        entries_[j - 1] = entries_[j];
                    --count_;
                    return;
                }
        }

        void receive(const T& msg) const noexcept { for (uint32_t i = 0; i < count_; ++i) entries_[i]->receive(msg); }

    private:
        Receiver* entries_[N] = {};
        uint32_t count_ = 0;
    };
} // END: sub0

#endif // CROG_SUB0PUB_WIRING_DYNAMIC_PORT_HPP
