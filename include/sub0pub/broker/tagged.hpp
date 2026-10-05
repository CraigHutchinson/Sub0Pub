/** Sub0Pub: Tagged<Data, Tag>: a distinct payload type whose tag carries its configuration
 * @remark Part of Sub0Pub (https://github.com/BareCpper/Sub0Pub), MIT License: see LICENSE.md.
 *         Included by the umbrella header <sub0pub/sub0pub.hpp>.
 */
#ifndef CROG_SUB0PUB_BROKER_TAGGED_HPP
#define CROG_SUB0PUB_BROKER_TAGGED_HPP

#include "sub0pub/config.hpp"
#include <type_traits>

namespace sub0
{
    /** A payload type made distinct by a tag, which also carries its configuration:
     *  `struct Rpm { using sub0_config = sub0::config<sub0::Capacity<2>>; }; using RpmMsg = sub0::Tagged<int, Rpm>;`
     */
    template<class Data, class Tag>
    struct Tagged
    {
        Data value;
    };
    namespace detail
    {
        template<class Tag, class = void> struct tag_config {};
        template<class Tag> struct tag_config<Tag, std::void_t<typename Tag::sub0_config>> { using type = typename Tag::sub0_config; };
    }
    template<class Data, class Tag>
    struct configure<Tagged<Data, Tag>> : detail::tag_config<Tag> {};
} // END: sub0

#endif // CROG_SUB0PUB_BROKER_TAGGED_HPP
