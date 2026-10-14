/** Sub0Pub: Bridge between the runtime broker and IPC: ForwardSubscribe, ForwardPublish and their *All forms
 * @remark Part of Sub0Pub (https://github.com/CraigHutchinson/Sub0Pub), MIT License: see LICENSE.md.
 *         Included by the umbrella header <sub0pub/sub0pub.hpp>.
 */
#ifndef CROG_SUB0PUB_IPC_FORWARD_HPP
#define CROG_SUB0PUB_IPC_FORWARD_HPP

#include "sub0pub/broker/subscribe.hpp"
#include "sub0pub/broker/publish.hpp"
#include "sub0pub/ipc/binary_writer.hpp"
#include "sub0pub/types.hpp"
#include <cstdint>
#include <tuple>

namespace sub0
{
    /** Forward receive() to  Target type convertible from this
     * @remark The call is made with Data type allowing for templated receive<>() handler functions @see class StreamSerializer
     * @note This uses the CRTP(curiously recurring template pattern) to forward to a target type derived from ForwardSubscribe<..>
     * @tparam  Data  Data type which will be forwarded to the derived Target implementation
     * @tparam  Target  Type of derived class which implements a function of type Target::receive<>( const Data& data ) via base inheritance or direct member
     */
    template<typename Data, typename Target >
    class ForwardSubscribe : public Subscribe<Data>
    {
    public:
        /** Receives subscribed data and forward to target object
         * @param data  Data to forward
         */
        inline void receive( const Data& data ) noexcept override
        {
            // Qualified, so the call is not virtual: the class Target::ForwardReceiver names, else Target itself
            if constexpr (requires { typename Target::ForwardReceiver; })
                static_cast<Target*>(this)->Target::ForwardReceiver::receive(data);
            else
                static_cast<Target*>(this)->Target::receive(data);
        }
    };

    /** Register publication of data with a provider instance
     * @remark The call is made with Data type allowing for templated receive<>() handler functions @see class StreamSerializer
     * @note This uses the CRTP(curiously recurring template pattern) to forward to a target type derived from ForwardPublish<..>
     * @tparam  Data  Data type which will be read into from a DataProvider
     * @tparam  DataProvider  CRTP Type of derived class which implements a function of type DataProvider::setDataPublisher( Data&, IPublish& ) via base inheritance or direct member
     */
    template<typename Data, typename DataProvider >
    class ForwardPublish : public Publish<Data>, protected IPublish
    {
    public:
        /** Register publisher buffer with the data provider
         * @param typeName  Unique name given to the serialised data entry @note Replaces compiler generated name which is not portable
         */
        ForwardPublish(
#if SUB0PUB_TYPEIDNAME
            const uint32_t typeId = 0, const char* typeName = 0/*nullptr*/
#endif
        )
            : Publish<Data>(
#if SUB0PUB_TYPEIDNAME
                typeId, typeName
#endif
              )
            , IPublish()
        {
            DataProvider& provider = static_cast<DataProvider&>(*this);
            provider.setDataPublisher( buffer_, static_cast<IPublish&>(*this) ); // Register the buffer sink to the data provider
        }

    private:

        /** Publish the data populated in buffer_
         */
        virtual void publish() final
        { Publish<Data>::publish( buffer_ ); }

    private:
        Data buffer_ = {}; ///< Data buffer to be published
    };

    namespace detail
    {
        template<class T> struct IsTuple : std::false_type {};
        template<class... Ts> struct IsTuple<std::tuple<Ts...>> : std::true_type {};

        /// The forms below took a std::tuple as a list until v2.0: reject one, so such code does not compile to a
        /// forwarder of the tuple type instead
        template<class... Datas>
        inline constexpr bool cNoTupleList = !(IsTuple<Datas>::value || ...);
    }

    /** Forward receive() to the Target for each message type listed (ForwardSubscribe<Data, Target> per type)
     * @remark The call is made with Data type allowing for templated receive<>() handler functions @see `class StreamSerializer` for example `template receive<>()`
     * @tparam  SubscriberTarget  CRTP type of the derived class, which implements Target::receive<>( const Data& data )
     * @tparam  Datas  Message types, or sub0::Types<...> lists of them, expanded in place
     */
    template<typename SubscriberTarget, typename... Datas>
    class ForwardSubscribeAll : public ForwardSubscribe<Datas, SubscriberTarget>...
    {
        static_assert(detail::cNoTupleList<Datas...>, "sub0pub: pass a list of message types as sub0::Types<...>; "
                      "a std::tuple is not a list here (ForwardSubscribe<std::tuple<...>, Target> forwards a tuple message)");
    };

    /// ForwardSubscribeAll with a sub0::Types list among its message types: the list's members, in its place
    template<typename SubscriberTarget, typename... Datas>
        requires detail::cAnyTypes<Datas...>
    class ForwardSubscribeAll<SubscriberTarget, Datas...>
        : public detail::Apply<::sub0::ForwardSubscribeAll, typename detail::Flatten<Datas...>::type, SubscriberTarget>::type
    {};

    /** Publish each message type listed from the DataProvider (ForwardPublish<Data, DataProvider> per type)
     * @tparam  DataProvider  CRTP type of the derived class, which implements setDataPublisher( Data&, IPublish& )
     * @tparam  Datas  Message types, or sub0::Types<...> lists of them, expanded in place
     */
    template<typename DataProvider, typename... Datas>
    class ForwardPublishAll : public ForwardPublish<Datas, DataProvider>...
    {
        static_assert(detail::cNoTupleList<Datas...>, "sub0pub: pass a list of message types as sub0::Types<...>; "
                      "a std::tuple is not a list here (ForwardPublish<std::tuple<...>, Provider> publishes a tuple message)");
    };

    /// ForwardPublishAll with a sub0::Types list among its message types: the list's members, in its place
    template<typename DataProvider, typename... Datas>
        requires detail::cAnyTypes<Datas...>
    class ForwardPublishAll<DataProvider, Datas...>
        : public detail::Apply<::sub0::ForwardPublishAll, typename detail::Flatten<Datas...>::type, DataProvider>::type
    {};
} // END: sub0

#endif // CROG_SUB0PUB_IPC_FORWARD_HPP
