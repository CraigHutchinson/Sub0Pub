/** Sub0Pub: Route<Data, Transport>: a transport endpoint bound to a table (egress, ingress, split horizon)
 * @remark Part of Sub0Pub (https://github.com/BareCpper/Sub0Pub), MIT License: see LICENSE.md.
 *         Included by the umbrella header <sub0pub/sub0pub.hpp>.
 */
#ifndef CROG_SUB0PUB_BROKER_ROUTE_HPP
#define CROG_SUB0PUB_BROKER_ROUTE_HPP

#include "sub0pub/broker/subscribe.hpp"
#include <type_traits>

namespace sub0
{
    /** Endpoint binding: connects one application-owned Transport instance to one table (a Domain, or the global
     * table) for one Data type.
     *
     *   Egress:  every message published into the table is handed to transport.send(data) -> SendResult.
     *            The transport must copy/serialize at acceptance and never retain a reference to `data`.
     *   Ingress: inject(data) publishes a message received from the transport into the table. That message is
     *            not sent back out through this route (split horizon), preventing echo loops between peers.
     *   Teardown: the destructor disconnects first, so after destruction the transport is never called.
     *
     * Transport concept: SendResult send(const Data&) noexcept.
     */
    template<class Data, class Transport>
    class Route final : public Subscribe<Data>
    {
        using Config = config_t<Data>;
        static_assert(Config::context != Context::None, "sub0pub: Route needs a publish context (split horizon and reports)");
    public:
        template<class C = Config, std::enable_if_t<C::storage == Storage::Global, int> = 0>
        explicit Route(Transport& transport) noexcept : transport_(transport) { this->trySubscribe(); }

        template<class C = Config, std::enable_if_t<C::storage == Storage::Scoped, int> = 0>
        Route(Domain<Data>& domain, Transport& transport) noexcept : Subscribe<Data>(domain), transport_(transport) { this->trySubscribe(); }

        ~Route() { this->disconnect(); }

        /// Ingress: deliver a message received from the transport to this route's table
        void inject(const Data& data) const noexcept { this->injectFrom(this, data); }

    private:
        void receive(const Data& data) noexcept override
        {
            const detail::Frame<Data>* const frame = kit::activeDispatch<Data>();
            if (frame != nullptr && frame->origin == this)
                return; // split horizon: this message arrived through this route
            const SendResult result = transport_.send(data);
            if (frame != nullptr && frame->report != nullptr)
                frame->report->record(result);
        }

        Transport& transport_;
    };
} // END: sub0

#endif // CROG_SUB0PUB_BROKER_ROUTE_HPP
