/** Sub0Pub: IPC receive buffers: Buffer and BufferRegister
 * @remark Part of Sub0Pub (https://github.com/BareCpper/Sub0Pub), MIT License: see LICENSE.md.
 *         Included by the umbrella header <sub0pub/sub0pub.hpp>.
 */
#ifndef CROG_SUB0PUB_IPC_BUFFER_REGISTER_HPP
#define CROG_SUB0PUB_IPC_BUFFER_REGISTER_HPP

#include "sub0pub/ipc/binary_writer.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <utility>

namespace sub0
{
    struct Buffer
    {
        IPublish* publisher; ///< Type specific publish of buffer
        char* buffer; ///< Data buffer @note a nullptr buffer may be set for unsupported payloads where paddingSize != 0 is required
        uint_least16_t bufferSize; ///< size of buffer
        int32_t paddingSize; /**< size of buffer padding data to ignore after buffer
                                  * @note Negative pad leaves unopulated bytes in buffer which are zeroed
                                  * @note For protocol version compatibility when payloads grow
                                  */
    };

    /** @tparam  cMaxDataBufferCount  Defines the maximum number of Data type buffers the deserializer can store
    */
    template< typename Header_t, uint_fast16_t cMaxDataBufferCount = 64U >
    class BufferRegister
    {
        typedef std::pair<Header_t,Buffer> HeaderToBuffer;
        typedef std::array<HeaderToBuffer, cMaxDataBufferCount> HeaderToBufferLookup;

    public:
        BufferRegister()
            : registry_()
            , registryEnd_(registry_.begin())
        {}

        /** Register a sink to the specified typed Data buffer
         * @remark Performs insertion sorting on buffers by the IPublish::typeId() for the buffer
         * @remark Called by sub0::ForwardPublish<Data>
         *
         * @param[in] publisher  Buffer handling object to store and signal data completion
         * @param[in] paddingSize  Number of trailing bytes after sizeof(Data) has been consumed to ignore/discard
         *                         for alignment or protocol-version compatibility
         */
        template < typename Data >
        void set(Data& buffer, IPublish& publisher, const int32_t paddingSize = 0U )
        {
            set( Header_t(buffer)
               , Buffer{
                     &publisher
                    , reinterpret_cast<char*>(&buffer)
                    , static_cast<uint_least16_t>(sizeof(buffer))
                    , paddingSize
               } );
        }

        void set(const Header_t& header, const Buffer& buffer)
        {
            const bool stored = trySet(header, buffer);
#if SUB0PUB_ASSERT
            assert(stored); // Capacity reached: use trySet() to handle exhaustion.
#endif
            (void)stored;
        }

        /// Insert or replace a buffer. A new entry at capacity returns false without changing the registry.
        /// Replacement remains valid at capacity. As with set(), callers serialize access.
        bool trySet(const Header_t& header, const Buffer& buffer)
        {
            typename HeaderToBufferLookup::iterator iInsert = std::lower_bound(std::begin(registry_), registryEnd_, header,
                [](const HeaderToBuffer& lhs, const Header_t& rhs) { return lhs.first < rhs; });

            const bool exists = (iInsert != registryEnd_) && (iInsert->first == header);
            if (!exists) //< Insert new entry at location
            {
                if (registryEnd_ == registry_.end())
                    return false;
                std::move_backward(iInsert, registryEnd_, registryEnd_ + 1U);
                ++registryEnd_;
                iInsert->first = header;
            }

            iInsert->second = buffer;

            if ( buffer.paddingSize < 0 ) //< Nullify unpopulated bytess
            {
                char* bufferEnd = buffer.buffer + buffer.bufferSize;
                std::fill(bufferEnd + buffer.paddingSize, bufferEnd, '\0'); //< Clear content that will not be written
            }
            return true;
        }

        Buffer find(const Header_t header)
        {
            typename HeaderToBufferLookup::iterator iFind = std::lower_bound(std::begin(registry_), registryEnd_, HeaderToBuffer(header, Buffer())
                , [](const HeaderToBuffer& lhs, const HeaderToBuffer& rhs) { return lhs.first < rhs.first; });

            if ((iFind != registryEnd_) && (iFind->first == header))
                return iFind->second;
            else
                return { nullptr, nullptr, 0U , 0U };
        }

        /** Default validation check against provided header
         * @note No validation occurs by default and processing is pushed onto find() to perform respective lookup operation
         * @param header Header data to validate against
         * @return True always
        */
        bool validate(const Header_t& /*header*/) const
        {
            return true;
        }

        bool close()
        {
            /** Do nothing - no state to clear */
            return true;
        }

    private:
        HeaderToBufferLookup registry_;
        typename HeaderToBufferLookup::iterator registryEnd_; ///< Iterator to end of registry_ @note Count = registryEnd_-registry_
    };
} // END: sub0

#endif // CROG_SUB0PUB_IPC_BUFFER_REGISTER_HPP
