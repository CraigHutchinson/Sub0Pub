/** Sub0Pub: StreamSerializer and StreamDeserializer
 * @remark Part of Sub0Pub (https://github.com/BareCpper/Sub0Pub), MIT License: see LICENSE.md.
 *         Included by the umbrella header <sub0pub/sub0pub.hpp>.
 */
#ifndef CROG_SUB0PUB_IPC_STREAM_HPP
#define CROG_SUB0PUB_IPC_STREAM_HPP

#include "sub0pub/ipc/protocol.hpp"
#include "sub0pub/utility/streams.hpp"
#include <type_traits>

namespace sub0
{
    /** Serialises Sub0Pub data into a target stream object
     * @remark Serialised data can be received and published using the counterpart StreamDeserializer instance
     * @remark Can be used to create inter-process transfers very easily using the specified Protocol @see sub0::DefaultSerialisation
     * @tparam  Protocol  Stream data protocol to use defining how the data header and payload is structured
     */
    template< typename Protocol = DefaultSerialisation, typename ProtocolWriter = typename Protocol::Writer >
    class StreamSerializer
    {
    public:

        using WriterConfig = typename ProtocolWriter::Config;

        using ForwardReceiver = StreamSerializer<Protocol,ProtocolWriter>; //<@note Allow disambiguation for forwarding from derived classes

    public:
        /** Construct from stream
         * @param[in] stream  Stream reference stored and used to write serialised data into
         */
        StreamSerializer( OStream& stream )
            : ostream_(stream)
            , writer_()
        {}

        bool configure( const WriterConfig& config )
        {
            if constexpr (!std::is_same_v<WriterConfig, detail::Empty>)
                return writer_.configure(ostream_, config);
            else
                return true;
        }

        /** Receives forwarded data from a subscriber and serialises it to the output stream
         * @param[in] data  Forwarded data
         */
        template<typename Data>
        void receive( const Data& data )
        {
            writer_.write( ostream_, data );
        }

        bool open()
        {
            return writer_.open(ostream_);
        }

        bool update()
        {
            return writer_.update(ostream_);
        }

        /** Reset writer internal  state
        */
        bool close()
        {
            writer_.close( ostream_ );
            ostream_.flush();
            return true;
        }

    protected:
        OStream& ostream_; ///< Stream into which data is serialised
        ProtocolWriter writer_;
    };


    /** Publishes messages from a serialised-input stream using the specified Protocol 
     * @remark StreamDeserializer can be used for inter-process or distributed systems over a network where the stream
     *  could be a TcpStream or could be a file in simple cases. The serialised data is expected to be generated from a
     *  corresponding StreamSerializer instance for the same Protocol.
     * @tparam  Protocol  Stream data protocol to use defining how the data header and payload is structured
     */
    template< typename Protocol = DefaultSerialisation, typename ProtocolReader = typename Protocol::Reader >
    class StreamDeserializer
    {
    public:

        using ReaderConfig = typename ProtocolReader::Config;

    public:
        /** Store reference to supplied IStream which will be read on update()
        */
        StreamDeserializer( IStream& istream )
            : istream_(istream)
            , reader_()
        {}

        bool configure(const ReaderConfig& config)
        {
            if constexpr (!std::is_same_v<ReaderConfig, detail::Empty>)
                return reader_.configure(istream_, config);
            else
                return true;
        }

        template < typename Data >
        void setDataPublisher( Data& dataBuffer, IPublish& publisher )
        {
            reader_.setDataPublisher(dataBuffer, publisher );
        }

        /** Prime reader internal  state
        */
        bool open()
        {
            return reader_.open(istream_);
        }

        /** Polls data from the input istream
         * @return True when data packet(s) have been published, false if no completed packet was present in istream
         */
        bool update()
        {
            return reader_.update(istream_);
        }

        /** Reset reader internal  state
        */
        bool close()
        {
            return reader_.close( istream_ );
        }

    protected:
        IStream& istream_; ///< Stream from which data is de-serialized
        ProtocolReader reader_;
    };
} // END: sub0

#endif // CROG_SUB0PUB_IPC_STREAM_HPP
