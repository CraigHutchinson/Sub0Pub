/** Sub0Pub: Bridge between the runtime broker and IPC: ForwardSubscribe, ForwardPublish and their *All forms
 * @remark Part of Sub0Pub (https://github.com/BareCpper/Sub0Pub), MIT License: see LICENSE.md.
 *         Included by the umbrella header <sub0pub/sub0pub.hpp>.
 */
#ifndef CROG_SUB0PUB_IPC_FORWARD_HPP
#define CROG_SUB0PUB_IPC_FORWARD_HPP

#include "sub0pub/broker/subscribe.hpp"
#include "sub0pub/broker/publish.hpp"
#include "sub0pub/ipc/binary_writer.hpp"
#include "sub0pub/utility/traits.hpp"
#include <cstdint>
#include <tuple>

namespace sub0
{
    /** Check for `Target::ForwardReceiver` for SFINAE 
    */
    template<typename Target>
    using forward_receiver_t = typename Target::ForwardReceiver;

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
            // Qualified, so the call is not virtual: the class that forward_receiver_t names, else Target itself.
            // maybe_unused: Clang's -Wunused-local-typedef does not count the qualified call as a use
            using ForwardReceiver_t [[maybe_unused]] = utility::detected_or_t<Target, forward_receiver_t, Target>;
            static_cast<Target*>(this)->ForwardReceiver_t::receive(data);
        }
    };

    /** Register publication of data with a provider instance
     * @remark The call is made with Data type allowing for templated receive<>() handler functions @see class StreamSerializer
     * @note This uses the CRTP(curiously recurring template pattern) to forward to a target type derived from ForwardPublish<..>
     * @tparam  Data  Data type which will be read into from a DataProvider
     * @tparam  DataProvider  CRTP Type of derived class which implements a function of type DataProvider::setDataPublisher( Data&, IPublish& ) via base inheritance or direct member
     *
     * @todo API not final
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
                      ///< @todo Double-buffer data storage for asynchronous processing and receive?
    };

    /** Forward receive() to Target type convertible from this for all Datas types listed
     * @remark The call is made with Data type allowing for templated receive<>() handler functions @see `class StreamSerializer` for example `template receive<>()`
     * @note This uses the CRTP(curiously recurring template pattern) to forward to a target type derived from ForwardSubscribe<..>
     * @tparam  Data  Data type which will be forwarded to the derived Target implementation
     * @tparam  Target  Type of derived class which implements a function of type Target::receive<>( const Data& data ) via base inheritance or direct member
     */
    template< typename SubscriberTarget, typename... Datas >
    class ForwardSubscribeAll : public ForwardSubscribe<Datas, SubscriberTarget>... {};

    template<typename SubscriberTarget, typename... Datas>
    class ForwardSubscribeAll<SubscriberTarget, std::tuple<Datas...> > : public ForwardSubscribe<Datas, SubscriberTarget>... {};


    /** Register publication of data with a provider instance
    */
    template< typename DataProvider, typename... Datas >
    class ForwardPublishAll : public ForwardPublish<Datas, DataProvider>... {};

    template<typename DataProvider, typename... Datas>
    class ForwardPublishAll<DataProvider, std::tuple<Datas...> > : public ForwardPublish<Datas, DataProvider>... {};
} // END: sub0

#endif // CROG_SUB0PUB_IPC_FORWARD_HPP
