/** Sub0Pub: Debug-build detection of a Data type resolved to different configurations (SUB0PUB_CHECK_CONFIG)
 * @remark Part of Sub0Pub (https://github.com/BareCpper/Sub0Pub), MIT License: see LICENSE.md.
 *         Included by the umbrella header <sub0pub/sub0pub.hpp>.
 */
#ifndef CROG_SUB0PUB_BROKER_CONFIG_CHECK_HPP
#define CROG_SUB0PUB_BROKER_CONFIG_CHECK_HPP

#include "sub0pub/config.hpp"
#include "sub0pub/utility/hash.hpp"
#include <atomic>
#include <cstdint>

namespace sub0
{
    namespace detail
    {
        /** Fingerprint of a configuration's effective values (not its type name) */
        template<class Config>
        constexpr uint32_t configFingerprint() noexcept
        {
            uint32_t h = 5381U;
            const uint32_t fields[] = {
                Config::capacity,
                static_cast<uint32_t>(Config::dispatch),
                static_cast<uint32_t>(Config::context),
                static_cast<uint32_t>(Config::storage),
                Config::filter ? 1U : 0U,
                utility::typeHash<typename Config::Lock>()
            };
            for (uint32_t f : fields)
                h = ((h << 5) + h) ^ f;
            return h | 1U; // never 0, which marks "unregistered"
        }

        /** Best-effort debug diagnostic for inconsistent configuration visibility across translation units
         * @warning Resolving a Data type differently in two TUs is an ODR violation and therefore undefined behaviour.
         *          Consistent visibility is a build contract; this registry only reports violations it observes.
         * @remark Registry<Data> does not depend on the configuration, so it is shared by all TUs.
         */
        template<class Data>
        struct Registry
        {
            inline static std::atomic<uint32_t> fingerprint{0};
        };

        template<class Data, class Config>
        void checkConfig() noexcept
        {
#if SUB0PUB_CHECK_CONFIG
            constexpr uint32_t mine = configFingerprint<Config>();
            uint32_t seen = 0;
            if (!Registry<Data>::fingerprint.compare_exchange_strong(seen, mine, std::memory_order_relaxed) && seen != mine)
                SUB0PUB_CONFIG_MISMATCH("sub0pub: Data type resolved to different configurations in different translation units");
#endif
        }
    } // END: detail
} // END: sub0

#endif // CROG_SUB0PUB_BROKER_CONFIG_CHECK_HPP
