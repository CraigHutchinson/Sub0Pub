/** Sub0Pub: Compile-time hashing and type identity: FourCC, hash(), typeHash<T>()
 * @remark Part of Sub0Pub (https://github.com/BareCpper/Sub0Pub), MIT License: see LICENSE.md.
 *         Included by the umbrella header <sub0pub/sub0pub.hpp>.
 */
#ifndef CROG_SUB0PUB_UTILITY_HASH_HPP
#define CROG_SUB0PUB_UTILITY_HASH_HPP

#include "sub0pub/config_macros.hpp"
#include <cstdint>

namespace sub0
{
    namespace utility
    {
        /** Create 4byte packed value at compile time
         * @tparam a,b,c,d  Characters which will be packed into 4-byte uint32_t value
         */
        template <const uint8_t a, const uint8_t b, const uint8_t c, const uint8_t d>
        struct FourCC
        {
            static constexpr uint32_t value = (((((d << 8) | c) << 8) | b) << 8) | a;
        };

        /** Hash a string using djb2 hash
         * @param[in] str  Null-terminated string to calculate hash of
         * @return djb2 hash value for input 'str'
         */
        constexpr uint32_t hash(const char* str)
        {
            uint32_t h = 5381U;
            for ( ; str[0U] != '\0'; ++str)
                h = ((h << 5) + h) + static_cast<uint32_t>(str[0U]);
            return h;
        }

        /** Compile-time unique type identifier using __PRETTY_FUNCTION__ / __FUNCSIG__
         * @tparam T  Type to generate a unique ID for
         * @return Unique uint32_t hash for type T, stable within a single build
         */
        template<typename T>
        constexpr uint32_t typeHash()
        {
#if defined(__GNUC__) || defined(__clang__)
            return hash(__PRETTY_FUNCTION__);
#elif defined(_MSC_VER)
            return hash(__FUNCSIG__);
#else
            static_assert(false, "Sub0Pub: typeHash requires GCC, Clang, or MSVC");
#endif
        }
    } // END: utility
} // END: sub0

#endif // CROG_SUB0PUB_UTILITY_HASH_HPP
