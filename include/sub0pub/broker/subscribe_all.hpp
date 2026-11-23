/** Sub0Pub: SubscribeAll<Datas...>: subscribe to several Data types at once
 * @remark Part of Sub0Pub (https://github.com/CraigHutchinson/Sub0Pub), MIT License: see LICENSE.md.
 *         Included by the umbrella header <sub0pub/sub0pub.hpp>.
 */
#ifndef CROG_SUB0PUB_BROKER_SUBSCRIBE_ALL_HPP
#define CROG_SUB0PUB_BROKER_SUBSCRIBE_ALL_HPP

#include "sub0pub/broker/subscribe.hpp"
#include "sub0pub/types.hpp"
#include <cstddef>

namespace sub0
{
    /** Subscribe to several message types at once: one Subscribe<Data> base per type, in order
     *
     * Every argument is a message type, a std::tuple included. A list of message types kept under one name is a
     * sub0::Types<...>, whose members are subscribed as if they were written in its place:
     *
     *     class Monitor : public sub0::SubscribeAll<SensorData, StatusCode> { ... };
     *     using HubMessages = sub0::Types<SensorData, StatusCode>;
     *     class Recorder : public sub0::SubscribeAll<HubMessages, Alarm> { ... };   // three types
     *
     * @remark Each Data type keeps its own topology: a receiver may be called directly for one type (StaticTo) and
     *         subscribe at run time for another. The class asks for the packed empty-base layout, so a receiver
     *         of statically wired types only is as small as a class without these bases.
     */
    template<class... Datas>
    class SUB0PUB_EMPTY_BASES SubscribeAll : public Subscribe<Datas>...
    {
    public:
        static constexpr size_t Count = sizeof...(Datas);
    };

    /// SubscribeAll with a sub0::Types list among its arguments: the list's members, in its place
    template<class... Datas>
        requires detail::cAnyTypes<Datas...>
    class SubscribeAll<Datas...> : public detail::Apply<::sub0::SubscribeAll, typename detail::Flatten<Datas...>::type>::type
    {};
} // END: sub0

#endif // CROG_SUB0PUB_BROKER_SUBSCRIBE_ALL_HPP
