/** Sub0Pub: IPC framing in: BinaryReader
 * @remark Part of Sub0Pub (https://github.com/BareCpper/Sub0Pub), MIT License: see LICENSE.md.
 *         Included by the umbrella header <sub0pub/sub0pub.hpp>.
 */
#ifndef CROG_SUB0PUB_IPC_BINARY_READER_HPP
#define CROG_SUB0PUB_IPC_BINARY_READER_HPP

#include "sub0pub/ipc/buffer_register.hpp"
#include "sub0pub/utility/streams.hpp"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <type_traits>

namespace sub0
{
    template< typename Prefix_t, typename Header_t, typename Postfix_t, typename BufferRegister = BufferRegister<Header_t> >
    class BinaryReader
    {
    public:
        using Config = detail::Empty; //< Not configurable by default

        enum class State { 
              Prefix///< [optional] Prefix-Delimiter is being read
            , Header ///< Data-Header  is being read
            , Data ///< Data payload is being  read
            , Postfix ///< [optional] Postfix-Delimiter is being read

            , SyncLost ///< Error state entered when an error occurs in any state i.e. Corrupted input stream

            , COUNT_ 
        };

    public:
        BinaryReader()
            : dataBufferRegistry_()
            , currentBuffer_()
            , state_()
            , prefix_()
            , header_()
            , postfix_()
        {}

        /** Initialise from IStream
        */
        bool open(IStream& /*stream*/)
        {
            //TODO: Do this on open or close?
            state_ = !std::is_void<Prefix_t>::value ? State::Prefix : stateAfter(State::Prefix);
            currentBuffer_ = findStateBuffer(state_);
            return true;
        }

        /** Read from the stream
        */
        bool update(IStream& stream)
        {
            for (;;)
            {
                // Handle SyncLost: scan for next valid prefix
                if (state_ == State::SyncLost)
                {
                    if (!tryResync(stream))
                        return false;
                }

                // Handle skip of unknown payload (data + postfix bytes)
                if (skipRemaining_ > 0)
                {
                    char skipBuf[256];
                    const auto toSkip = std::min(static_cast<uint32_t>(sizeof(skipBuf)), skipRemaining_);
#if SUB0PUB_STD
                    const auto skipped = static_cast<uint32_t>(stream.read(skipBuf, toSkip).gcount());
#else
                    const auto skipped = static_cast<uint32_t>(stream.read(skipBuf, toSkip));
#endif
                    skipRemaining_ -= skipped;
                    if (skipRemaining_ > 0)
                        return false;
                    // Skip complete — reset to next prefix
                    state_ = !std::is_void_v<Prefix_t> ? State::Prefix : stateAfter(State::Prefix);
                    currentBuffer_ = findStateBuffer(state_);
                    continue;
                }

                // Normal read
                if (!readBuffer(stream))
                    return false;
                if (state_ == State::Header)
                    return true;
            }
        }

        template < typename Data >
        void setDataPublisher(Data& dataBuffer, IPublish& publisher)
        {
#if SUB0PUB_ASSERT
            assert(!currentBuffer_.buffer); /// @todo We don't intend to support adding buffers while stream is being processed?
#endif
            dataBufferRegistry_.set(dataBuffer, publisher);
        }

        bool close( IStream& /*stream*/ )
        {
            dataBufferRegistry_.close(); ///< @TODO This is here as a use-case contained stream state wihin the buffer map! Remove/deprecate this when/as possible
            return true;
        }

    private:

        /** Returns/finds buffer for state
        */
        Buffer findStateBuffer(const State state)
        {
            switch (state)
            {
            default: //< @todo unreachable unless SyncLost
            case State::Prefix: 
                return {nullptr, reinterpret_cast<char*>(&prefix_), static_cast<uint_least16_t>( !std::is_void<Prefix_t>::value ? sizeof(prefix_) : 0U), 0U};
            case State::Header: 
                return {nullptr, reinterpret_cast<char*>(&header_), static_cast<uint_least16_t>(sizeof(header_)), 0U };
            case State::Data:   
                return dataBufferRegistry_.find(header_);
            case State::Postfix: 
                return {currentBuffer_.publisher , reinterpret_cast<char*>(&postfix_), static_cast<uint_least16_t>( !std::is_void<Postfix_t>::value ? sizeof(postfix_) : 0U), 0U};
            }
        }
        
        /** Read payload data from stream and detect payload completion
         * @return True when data packet(s) have been published, false if no completed packet was present in stream
        */
        bool readBuffer(IStream& stream)
        {
            if (currentBuffer_.bufferSize > 0)
            {
#if SUB0PUB_STD
                const uint_fast16_t readCount = static_cast<uint_fast16_t>(stream.read(currentBuffer_.buffer, currentBuffer_.bufferSize).gcount()); ///< @todo readsome() for async
#else
                const uint_fast16_t readCount = stream.read(currentBuffer_.buffer, currentBuffer_.bufferSize);
#endif
                currentBuffer_.buffer += readCount;
                currentBuffer_.bufferSize -= static_cast<uint_least16_t>(readCount); // read() returns at most bufferSize

                /// If buffer not complete then we need to return and await more data
                if (currentBuffer_.bufferSize > 0)
                    return false;
            }

            if (currentBuffer_.paddingSize > 0)
            {
                char ignoreBuff[256];
                const size_t ignoreSize = std::min(std::size(ignoreBuff), static_cast<size_t>(currentBuffer_.paddingSize));
    #if SUB0PUB_STD
                const uint_fast16_t ignoreCount = static_cast<uint_fast16_t>(stream.read(ignoreBuff, ignoreSize).gcount());
    #else
                const uint_fast16_t ignoreCount = stream.read(ignoreBuff, static_cast<IStream::StreamSize>(ignoreSize)); // at most sizeof(ignoreBuff)
    #endif

                currentBuffer_.paddingSize -= ignoreCount;

                /// If padding not complete then we need to return and await more data
                /// @todo We could publish the data before completion of the padding... however we cannot check for a post-fix delimiter without doing pad first!?
                if (currentBuffer_.paddingSize > 0)
                    return false;
            }

            //If we got here the buffer and any padding has been read from the stream
            return stateComplete();
        }

        constexpr bool getStateStatus(const State state) const
        {
            switch (state)
            {
            default:
            case State::Prefix:
                if constexpr (!std::is_void_v<Prefix_t>)
                {
                    const Prefix_t emptyPrefix{}; // Named: taking the address of a temporary is ill-formed (GCC hard error)
                    return std::memcmp(&prefix_, &emptyPrefix, sizeof(Prefix_t)) == 0;
                }
                else
                    return true;
            case State::Header:  return dataBufferRegistry_.validate(header_);
            case State::Data:    return true;
            case State::Postfix:
                if constexpr (std::is_void_v<Postfix_t>)
                    return true;
                else
                    return postfix_ == Postfix_t();
            }
        }

        constexpr bool isPublishReady(const State currentState) const
        {
            // @todo In absence of Postfix we should probably wait for Prefix instead of just Data completion?
            return currentState == (!std::is_void<Postfix_t>::value ? State::Postfix : State::Data);
        }

        constexpr State stateAfter(const State currentState ) const
        {
            switch (currentState)
            {
            default: //< @todo unreachable
            case State::Prefix:  return State::Header;
            case State::Header:  return State::Data;
            case State::Data:    return !std::is_void<Postfix_t>::value ? State::Postfix : stateAfter(State::Postfix); ///< @note may not have Prefix_t or Postfix_t
            case State::Postfix: return !std::is_void<Prefix_t>::value ? State::Prefix : stateAfter(State::Prefix);
            }
        }

        bool checkStatusOfState(const State currentState)
        {
            const bool stateStatus = getStateStatus(currentState);
            if(stateStatus)
                return true;

            const char* failureMessage = nullptr;
            switch(currentState)
            {
                case State::Header: failureMessage = "Binary-Header mismatch - stream corruption or incompatible data-stream"; break;
                case State::Postfix: failureMessage = "Binary-Postfix mismatch - stream corruption or incompatible data-stream"; break;
                default: failureMessage = "Sync-Lost - TODO Details"; break;
            }

            // A caller may catch the error and call update() again to resume at the next frame.
            state_ = State::SyncLost;

            if(failureMessage != nullptr)
            {
#if __cpp_exceptions
                throw std::runtime_error(failureMessage);
#elif SUB0PUB_ASSERT
                assert((void*)0 == failureMessage);
#endif
            }

            return false;
        }

        bool stateComplete()
        {
            if( !checkStatusOfState(state_) )
            {
                return false;
            }

            if ( isPublishReady(state_) )
            {
                if (currentBuffer_.publisher)
                    currentBuffer_.publisher->publish();
            }

            state_ = stateAfter( state_ );
            currentBuffer_ = findStateBuffer(state_);

            // Unknown typeId: skip the payload + postfix bytes and continue to next frame
            if (currentBuffer_.buffer == nullptr && state_ == State::Data)
            {
                skipRemaining_ = header_.dataBytes;
                if constexpr (!std::is_void_v<Postfix_t>)
                    skipRemaining_ += sizeof(Postfix_t);
                return true;
            }

            if (currentBuffer_.paddingSize < 0)
            {
                currentBuffer_.bufferSize = static_cast<uint_least16_t>(currentBuffer_.bufferSize + currentBuffer_.paddingSize);
                currentBuffer_.paddingSize = 0;
            }

            return currentBuffer_.buffer != nullptr;
        }

        /** Attempt to recover from SyncLost by scanning for the next valid prefix magic
         * @return True if magic found and state reset to Header, false if more data needed
         */
        bool tryResync(IStream& stream)
        {
            if constexpr (std::is_void_v<Prefix_t>)
            {
                // No prefix defined — cannot resync
                return false;
            }
            else
            {
                // Scan one byte at a time looking for the prefix magic
                auto* prefixBytes = reinterpret_cast<char*>(&prefix_);
                const auto prefixSize = sizeof(Prefix_t);
                const Prefix_t expected{};

                for (;;)
                {
                    char byte;
#if SUB0PUB_STD
                    const auto readCount = static_cast<uint_fast16_t>(stream.read(&byte, 1).gcount());
#else
                    const auto readCount = stream.read(&byte, 1);
#endif
                    if (readCount == 0)
                        return false;

                    // Shift prefix buffer left and append new byte
                    std::memmove(prefixBytes, prefixBytes + 1, prefixSize - 1);
                    prefixBytes[prefixSize - 1] = byte;

                    // Check if we've found the magic
                    if (std::memcmp(&prefix_, &expected, prefixSize) == 0)
                    {
                        state_ = State::Header;
                        currentBuffer_ = findStateBuffer(state_);
                        return true;
                    }
                }
            }
        }

    private:
        BufferRegister dataBufferRegistry_;
        Buffer currentBuffer_; ///< Current prefix/header/payload/postfix buffer
        State state_; ///< Which buffer is being read
        uint32_t skipRemaining_ = 0; ///< Bytes remaining to skip for unknown typeId payloads

        using MemberPrefix_t = std::conditional_t<std::is_void_v<Prefix_t>, char, Prefix_t>;
        using MemberPostfix_t = std::conditional_t<std::is_void_v<Postfix_t>, char, Postfix_t>;

        MemberPrefix_t prefix_;
        Header_t header_; ///< Packet head buffer
        MemberPostfix_t postfix_;
    };
} // END: sub0

#endif // CROG_SUB0PUB_IPC_BINARY_READER_HPP
