/** Sub0Pub: IPC framing out: IPublish and BinaryWriter
 * @remark Part of Sub0Pub (https://github.com/BareCpper/Sub0Pub), MIT License: see LICENSE.md.
 *         Included by the umbrella header <sub0pub/sub0pub.hpp>.
 */
#ifndef CROG_SUB0PUB_IPC_BINARY_WRITER_HPP
#define CROG_SUB0PUB_IPC_BINARY_WRITER_HPP

#include "sub0pub/config_macros.hpp"
#include "sub0pub/types.hpp"
#include "sub0pub/utility/streams.hpp"

namespace sub0
{
    /** Interface for data provider to indicate destination buffer status
     * @see ForwardPublish
     */
    class IPublish
    {
    public:

        /** Publish the data owned by the object
         */
        virtual void publish() = 0;
    };

    template< typename Prefix_t
            , typename Header_t
            , typename Postfix_t >
    class BinaryWriter
    {
    public:
        using Config = detail::Empty; //< Not configurable by default

    public:
        /** Output header and pay-load for data as binary
         * @param stream  Stream to write into
         * @param data  Data to construct a header record and data payload for
         */
        template<typename Data_t>
        inline bool write(OStream& stream, const Data_t& data) const
        {
            return utility::write<Prefix_t>(stream)
                && utility::write(stream, Header_t(data))
                && utility::write(stream, data)
                && utility::write<Postfix_t>(stream);
        }

        bool open(OStream& /*stream*/)
        {
            /* Do nothing */
            return true;
        }

        /// The default writer emits complete frames synchronously; there is no pending work to poll.
        bool update(OStream& /*stream*/) const noexcept { return true; }

        void close( OStream& /*stream*/ )
        {
            /* Do nothing */
        }

    };
} // END: sub0

#endif // CROG_SUB0PUB_IPC_BINARY_WRITER_HPP
