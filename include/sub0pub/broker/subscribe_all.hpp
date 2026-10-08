/** Sub0Pub: SubscribeAll<Datas...>: subscribe to several Data types at once
 * @remark Part of Sub0Pub (https://github.com/BareCpper/Sub0Pub), MIT License: see LICENSE.md.
 *         Included by the umbrella header <sub0pub/sub0pub.hpp>.
 */
#ifndef CROG_SUB0PUB_BROKER_SUBSCRIBE_ALL_HPP
#define CROG_SUB0PUB_BROKER_SUBSCRIBE_ALL_HPP

#include "sub0pub/broker/subscribe.hpp"
#include <cstddef>
#include <tuple>
#include <utility>

namespace sub0
{
    /**  Subscribe to many
    * @remark Each Data type keeps its own topology: a receiver may be called directly for one type (StaticTo) and
    *         subscribe at run time for another. The class asks for the packed empty-base layout, so a receiver
    *         of statically wired types only is as small as a class without these bases.
    * @remark A std::tuple argument is a list of Data types, not a Data type: to subscribe to a message that is itself
    *         a std::tuple, derive from Subscribe<std::tuple<...>> directly.
    */
    template< typename... Datas >
    class SUB0PUB_EMPTY_BASES SubscribeAll : public Subscribe<Datas>...
    {
    public:
        static constexpr size_t Count = sizeof...(Datas);
    };

    /**  Subscribe to many defined by std::tuple type list
    */
    template<typename... Datas>
    class SUB0PUB_EMPTY_BASES SubscribeAll<std::tuple<Datas...>> : public Subscribe<Datas>...
    {
    public:
        static constexpr size_t Count = sizeof...(Datas);
    };

    /** Subscribe to many defined by multiple std::tuple type i.e. SubscribeAll< std::tuple<A,B>, std::tuple<B,C> >
    */
    template<typename... Datas, typename... OtherTuples>
    class SubscribeAll<std::tuple<Datas...>, OtherTuples...>
        : public SubscribeAll< decltype(std::tuple_cat( std::declval<std::tuple<Datas...>>(), std::declval<OtherTuples>()...)) >
    {};
} // END: sub0

#endif // CROG_SUB0PUB_BROKER_SUBSCRIBE_ALL_HPP
