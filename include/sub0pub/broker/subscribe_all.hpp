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
    * @todo Specialisation on std::tuple exists and could cause unexpected expansion if this was a desired type being published!
    */
    template< typename... Datas >
    class SubscribeAll : public Subscribe<Datas>...
    {
    public:
        static constexpr size_t Count = sizeof...(Datas);
    };

    /**  Subscribe to many defined by std::tuple type list
    */
    template<typename... Datas>
    class SubscribeAll<std::tuple<Datas...>> : public Subscribe<Datas>...
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
