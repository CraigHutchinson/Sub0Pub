/** Sub0Pub: Detection idiom: is_detected, detected_t, detected_or_t
 * @remark Part of Sub0Pub (https://github.com/BareCpper/Sub0Pub), MIT License: see LICENSE.md.
 *         Included by the umbrella header <sub0pub/sub0pub.hpp>.
 */
#ifndef CROG_SUB0PUB_UTILITY_TRAITS_HPP
#define CROG_SUB0PUB_UTILITY_TRAITS_HPP

#include "sub0pub/config_macros.hpp"
#include <type_traits>

namespace sub0
{
    namespace utility
    {
        /// std::experimental::is_detected
        /// https://en.cppreference.com/w/cpp/experimental/is_detected
        namespace detail {
            template <class Default, class AlwaysVoid,
                template<class...> class Op, class... Args>
            struct detector {
                using value_t = std::false_type;
                using type = Default;
            };

            template <class Default, template<class...> class Op, class... Args>
            struct detector<Default, std::void_t<Op<Args...>>, Op, Args...> {
                using value_t = std::true_type;
                using type = Op<Args...>;
            };

        } // namespace detail

        struct nonesuch {
            ~nonesuch() = delete;
            nonesuch(nonesuch const&) = delete;
            void operator=(nonesuch const&) = delete;
        };

        template <template<class...> class Op, class... Args>
        using is_detected = typename detail::detector<nonesuch, void, Op, Args...>::value_t;

        template <template<class...> class Op, class... Args>
        using detected_t = typename detail::detector<nonesuch, void, Op, Args...>::type;

        template <class Default, template<class...> class Op, class... Args>
        using detected_or_t = typename detail::detector<Default, void, Op, Args...>::type;
    } // END: utility
} // END: sub0

#endif // CROG_SUB0PUB_UTILITY_TRAITS_HPP
