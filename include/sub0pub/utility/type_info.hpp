/** Sub0Pub: User-assigned type identity for streams (SUB0PUB_TYPEIDNAME), independent of configuration
 * @remark Part of Sub0Pub (https://github.com/BareCpper/Sub0Pub), MIT License: see LICENSE.md.
 *         Included by the umbrella header <sub0pub/sub0pub.hpp>.
 */
#ifndef CROG_SUB0PUB_UTILITY_TYPE_INFO_HPP
#define CROG_SUB0PUB_UTILITY_TYPE_INFO_HPP

#include "sub0pub/config_macros.hpp"
#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstring>

namespace sub0
{
    namespace detail
    {
#if SUB0PUB_TYPEIDNAME
        /** User-assigned identity of a Data type for inter-process streams (SUB0PUB_TYPEIDNAME)
         * @remark Independent of the configuration, so shared by all translation units
         */
        template<class Data>
        struct TypeInfo
        {
            /// Set once: publishers and subscribers may be constructed on several threads at once
            static void set(const uint32_t id, const char* const name) noexcept
            {
                if (id)
                {
                    uint32_t seen = 0;
                    if (!id_.compare_exchange_strong(seen, id, std::memory_order_acq_rel))
                    {
#if SUB0PUB_ASSERT
                        assert(seen == id); // a Data type must be given one identifier
#endif
                    }
                }
                if (name)
                {
                    const char* seen = nullptr;
                    if (!name_.compare_exchange_strong(seen, name, std::memory_order_acq_rel))
                    {
#if SUB0PUB_ASSERT
                        assert(std::strcmp(seen, name) == 0); // and one name
#endif
                    }
                }
            }

            static uint32_t typeId() noexcept { return id_.load(std::memory_order_acquire); }
            static const char* typeName() noexcept { return name_.load(std::memory_order_acquire); }

        private:
            inline static std::atomic<uint32_t> id_{0};
            inline static std::atomic<const char*> name_{nullptr};
        };
#endif
    } // END: detail
} // END: sub0

#endif // CROG_SUB0PUB_UTILITY_TYPE_INFO_HPP
