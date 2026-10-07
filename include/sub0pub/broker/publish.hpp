/** Sub0Pub: Publish<Data> and the free functions publish() and cancel()
 * @remark Part of Sub0Pub (https://github.com/BareCpper/Sub0Pub), MIT License: see LICENSE.md.
 *         Included by the umbrella header <sub0pub/sub0pub.hpp>.
 */
#ifndef CROG_SUB0PUB_BROKER_PUBLISH_HPP
#define CROG_SUB0PUB_BROKER_PUBLISH_HPP

#include "sub0pub/broker/subscribe.hpp"
#include "sub0pub/utility/type_info.hpp"
#include "sub0pub/utility/streams.hpp"
#include <cassert>
#include <cstdint>
#include <type_traits>

namespace sub0
{
    /** Base type for an object that publishes to some strong-typed Data
     * @tparam  Data  Type that will be published by this object to subscribers of corresponding type
     * @remark Not polymorphic: no virtual destructor and no vptr. The destructor is protected, so a publisher is a class
     *         derived from Publish<Data> and is never deleted through a Publish<Data>*. For Global storage it is an empty handle.
     * @remark Topology: this is the form for a type the runtime broker delivers, alone or after the receivers a
     *         StaticFirst list names. For a StaticTo type Publish<Data> is the specialisation below.
     */
    template<class Data>
    class Publish
    {
        using Config = config_t<Data>;
        using Broker = detail::BrokerFor<Data>;
    public:
        /** @param[in] typeId, typeName  Optional unique identity of Data for inter-process streams (SUB0PUB_TYPEIDNAME) */
        template<class C = Config, std::enable_if_t<C::storage == Storage::Global, int> = 0>
        Publish(
#if SUB0PUB_TYPEIDNAME
            const uint32_t typeId = 0, const char* typeName = nullptr
#endif
        ) noexcept
        {
#if SUB0PUB_TYPEIDNAME
            detail::TypeInfo<Data>::set(typeId, typeName);
#endif
        }

        /** Publish into `domain` (Scoped types) */
        template<class C = Config, std::enable_if_t<C::storage == Storage::Scoped, int> = 0>
        explicit Publish(Domain<Data>& domain) noexcept : broker_(domain.table_) {}

        Publish(const Publish&) = default;
        Publish& operator=(const Publish&) = default;

        /** Cancel the active publication of Data, stopping delivery to the remaining subscribers
         * @note Only meaningful from within a receive() callback
         */
        void cancel() const noexcept
        {
            static_assert(Config::context != Context::None,
                          "sub0pub: cancel() needs a publish context: define SUB0PUB_CANCEL, or configure the type with "
                          "sub0::ThreadLocalContext or sub0::StaticContext");
            broker_.cancel();
        }

        /** @return The number of receivers a publication of Data reaches at this moment: the subscribers registered
         *          now, plus the receivers a StaticFirst list names
         * @remark For a call site that decides for itself what an absent receiver means. A concurrent configuration
         *         reads it under the table's lock; it may have changed by the time the caller acts on it.
         */
        uint32_t receiverCount() const noexcept
        {
            uint32_t count = broker_.receivers();
            if constexpr (detail::cBridged<Data>)
                count += detail::topology_t<Data>::template cReceivers<Data>;
            return count;
        }

#if SUB0PUB_TYPEIDNAME
        /** @return Null-terminated name given to Data, or nullptr */
        const char* typeName() const noexcept { return detail::TypeInfo<Data>::typeName(); }

        /** @return Unique identifier given to Data, or 0 */
        uint32_t typeId() const noexcept { return detail::TypeInfo<Data>::typeId(); }

        /** Stream operator for diagnostics reporting */
        friend OStream& operator<< (OStream& stream, const Publish<Data>& publisher)
        { return stream << publisher.typeName() << '{' << (const void*)&publisher << '}'; }
#endif

    protected:
        /// Protected and non-virtual: a publisher is destroyed as its own type, never through a Publish<Data>*
        ~Publish() = default;

        /** Publish data to subscribers
         * @note Protected: use the free function sub0::publish(*this, data) from derived classes
         */
        void publish(const Data& data, PublishReport* report = nullptr) const noexcept
        {
            // StaticFirst: the receivers the type's list names are called directly, ahead of the runtime subscribers
            if constexpr (detail::cBridged<Data>)
                detail::topology_t<Data>::publish(data);
            broker_.publish(data, nullptr, report);
        }

    private:
        template<class From, class D> friend void publish(From&, const D&) noexcept;
        template<class From, class D> friend void publish(From&, const D&, PublishReport&) noexcept;
        Broker broker_;
    };

    /** Publish<Data> for a statically wired type (StaticTo): an empty handle
     * @tparam  Data  Type whose configuration names its receivers
     *
     * publish() calls the receivers in the type's StaticTo list directly, in the order of the list. There is no
     * subscription table, so there is nothing to register, lock or iterate; the publisher is written exactly as
     * for a brokered type.
     *
     * @remark cancel() and publish reports need the runtime broker and do not exist here.
     */
    template<class Data>
        requires detail::cWired<Data>
    class Publish<Data>
    {
    public:
#if SUB0PUB_TYPEIDNAME || SUB0PUB_CHECK_CONFIG
        /** @param[in] typeId, typeName  Optional unique identity of Data for inter-process streams (SUB0PUB_TYPEIDNAME) */
        Publish(
#if SUB0PUB_TYPEIDNAME
            const uint32_t typeId = 0, const char* typeName = nullptr
#endif
        ) noexcept
        {
#if SUB0PUB_TYPEIDNAME
            detail::TypeInfo<Data>::set(typeId, typeName);
#endif
            detail::checkConfig<Data, config_t<Data>>();
        }
#else
        Publish() = default;
#endif

        Publish(const Publish&) = default;
        Publish& operator=(const Publish&) = default;

        /** @return The number of receivers in the type's StaticTo list that receive Data: a constant of the program */
        static constexpr uint32_t receiverCount() noexcept { return detail::topology_t<Data>::template cReceivers<Data>; }

#if SUB0PUB_TYPEIDNAME
        /** @return Null-terminated name given to Data, or nullptr */
        const char* typeName() const noexcept { return detail::TypeInfo<Data>::typeName(); }

        /** @return Unique identifier given to Data, or 0 */
        uint32_t typeId() const noexcept { return detail::TypeInfo<Data>::typeId(); }
#endif

    protected:
        /// Protected and non-virtual: a publisher is destroyed as its own type, never through a Publish<Data>*
        ~Publish() = default;

        /** Publish data to the receivers the type's StaticTo list names
         * @note Protected: use the free function sub0::publish(*this, data) from derived classes
         */
        // An incomplete-type error here: include sub0pub/sub0pub.hpp (or sub0pub/wiring/static_topology.hpp)
        SUB0PUB_FORCE_INLINE void publish(const Data& data) const noexcept { detail::topology_t<Data>::publish(data); }

    private:
        template<class From, class D> friend void publish(From&, const D&) noexcept;
    };

    /** Publish data, used when inheriting from multiple Publish<> base types
     * @remark Circumvents C++ name hiding when multiple Publish<> bases are present (publish(1.0F) would be ambiguous)
     * @note Compile error if From does not inherit Publish<Data>
     * @param[in] from  Producer object inheriting from one or more Publish<> objects
     * @param[in] data  Data that will be published using the base Publish<Data> object of From
     */
    template<class From, class Data>
    SUB0PUB_FORCE_INLINE void publish(From& from, const Data& data) noexcept
    {
        const Publish<Data>& publisher = from;
        publisher.publish(data);
    }

    /** Publish and report route results (routed / accepted / rejected). Local delivery is unaffected by rejections. */
    template<class From, class Data>
    inline void publish(From& from, const Data& data, PublishReport& report) noexcept
    {
        static_assert(config_t<Data>::context != Context::None, "sub0pub: publish reports need a publish context");
        const Publish<Data>& publisher = from;
        publisher.publish(data, &report);
    }

    /** @see publish(From&, const Data&) */
    template<class From, class Data>
    SUB0PUB_FORCE_INLINE void publish(From* const from, const Data& data) noexcept
    {
#if SUB0PUB_ASSERT
        assert(from != nullptr);
#endif
        publish(*from, data);
    }

    /** The number of receivers a publication of Data by `from` reaches at this moment
     * @param[in] from  Producer object inheriting from Publish<Data>
     * @return Registered subscribers plus bound receivers; a constant for a StaticTo type
     * @remark Lets a call site treat an absent receiver as its own decision, for a type configured with
     *         AllowNoReceivers: `if (sub0::receiverCount<Reading>(*this) == 0) ...`
     */
    template<class Data, class From>
    inline uint32_t receiverCount(From& from) noexcept
    {
        const Publish<Data>& publisher = from;
        return publisher.receiverCount();
    }

    /** Cancel the active publication of Data on a publisher
     * @param[in] from  Producer object inheriting from Publish<Data>
     * @note Only meaningful from within a receive() callback
     */
    template<class Data, class From>
    inline void cancel(From& from) noexcept
    {
        const Publish<Data>& publisher = from;
        publisher.cancel();
    }
} // END: sub0

#endif // CROG_SUB0PUB_BROKER_PUBLISH_HPP
