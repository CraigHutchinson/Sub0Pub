/** Sub0Pub: Results reported by the runtime broker: SubscribeResult, SendResult, PublishReport
 * @remark Part of Sub0Pub (https://github.com/BareCpper/Sub0Pub), MIT License: see LICENSE.md.
 *         Included by the umbrella header <sub0pub/sub0pub.hpp>.
 */
#ifndef CROG_SUB0PUB_BROKER_RESULTS_HPP
#define CROG_SUB0PUB_BROKER_RESULTS_HPP

#include "sub0pub/config_macros.hpp"
#include <cstdint>

namespace sub0
{
    /** Outcome of a bounded subscription registration
     * @see Subscribe::trySubscribe, Subscribe::isSubscribed
     */
    enum class SubscribeResult : uint8_t
    {
        Subscribed,        ///< Registered; the subscriber receives subsequent publishes
        CapacityExceeded,  ///< The table was full; table left unchanged
        Closed             ///< The subscriber's Domain has been closed
    };

    /** Outcome of handing a message to a transport. Acceptance is NOT remote delivery. */
    enum class SendResult : uint8_t
    {
        Accepted,          ///< The transport took the message (copied or serialized it)
        Full,              ///< Temporary: queue or buffer exhausted
        Disconnected,      ///< No peer at the moment
        Closed             ///< The transport is shutting down or shut down
    };

    /** Opt-in per-publish report of route results: sub0::publish(from, data, report)
     * @remark Local delivery is not affected by route results: every local subscriber is still called when a route rejects.
     */
    struct PublishReport
    {
        uint32_t routed = 0;
        uint32_t accepted = 0;
        uint32_t rejected = 0;
        SendResult lastRejection = SendResult::Accepted;

        void record(SendResult r) noexcept
        {
            ++routed;
            if (r == SendResult::Accepted)
                ++accepted;
            else
            {
                ++rejected;
                lastRejection = r;
            }
        }
    };
} // END: sub0

#endif // CROG_SUB0PUB_BROKER_RESULTS_HPP
