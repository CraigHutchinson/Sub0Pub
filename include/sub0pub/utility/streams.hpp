/** Sub0Pub: Minimal stream interfaces for IPC: utility::OStream/IStream (or std streams with SUB0PUB_STD)
 * @remark Part of Sub0Pub (https://github.com/BareCpper/Sub0Pub), MIT License: see LICENSE.md.
 *         Included by the umbrella header <sub0pub/sub0pub.hpp>.
 */
#ifndef CROG_SUB0PUB_UTILITY_STREAMS_HPP
#define CROG_SUB0PUB_UTILITY_STREAMS_HPP

#include "sub0pub/config_macros.hpp"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>

namespace sub0
{
    namespace utility
    {
        /**
        * @note char* to unify interface against std::ostream
        */
        class OStream
        {
        public:
            typedef uint_fast32_t StreamSize;

            virtual StreamSize write(const char* const buffer, const StreamSize bufferCount) = 0;

            /** Clear all buffers for this stream and causes any buffered data to be written to the underlying device.
            */
            virtual void flush() = 0;
        };

        /**
        * @note char* to unify interface against std::istream
        */
        class IStream
        {
        public:
            typedef uint_fast32_t StreamSize;

            virtual StreamSize read(char* const buffer, const StreamSize bufferCount) = 0;

            /** Read stream line-by line until '\r', '\n', or '\r\n'
                @note Extends sub0::IStream
            */
            virtual StreamSize readline(char* const buffer, const StreamSize bufferCount) = 0;

            /** Discards specified number of characters from inputSequence
            * @note Setting std::numeric_limits<std::streamsize>::max() discards ONLY the currently buffered bytes
            * @return The number of bytes ignored
            */
            virtual StreamSize ignore( const StreamSize bufferCount ) = 0;

            /** Discards specified number of characters from inputSequence until the specified delimiter is found
            * @note The delimiting character is extracted, and thus the next input operation will continue on the character that follows it (if any).
            * @warning This function may (TBC) block if there isn't any data in the stream
            * @return The number of bytes ignored including the delimiter character
            */
            virtual StreamSize ignore(const StreamSize bufferCount, const char delimiter ) = 0;

            /** Returns whether end of stream has been reached
             * @note For Files this is explicit but for a pipe (e.g. TCP or command pipe '|' ) this may never occur until the pipe is forcefully closed by the other end etc
             * @return True if no more data, false otherwise
            */
            virtual bool isEof() = 0;
        };

// TODO: Need to refactor use of streams!?
#if SUB0PUB_STD
        /// @todo Determine how to avoid this i.e. Drop std::istream or only use interface type?
        inline size_t readline(std::istream& istream, char* const buffer, const size_t bufferCount)
        {
            return istream.getline(buffer, bufferCount).gcount();
        }

        template< typename Type_t >
        inline bool write(std::ostream& stream, const Type_t& value)
        {
            return stream.write(reinterpret_cast<const char*>(&value), sizeof(value)).good();
        }

        template< typename Type_t >
        inline bool write(std::ostream& stream)
        {
            const Type_t defaulted;
            return stream.write(reinterpret_cast<const char*>(&defaulted), sizeof(defaulted)).good();
        }

        template<>
        inline bool write<void>(std::ostream& /*stream*/)
        {
            return true;
        }
#else
        /// @todo Determine how to avoid this i.e. Drop std::istream or only use interface type?
        inline size_t readline(IStream& istream, char* const buffer, const size_t bufferCount)
        {
            if constexpr (sizeof(size_t) > sizeof(IStream::StreamSize)) // where the narrowing below can lose data
                assert(bufferCount <= (std::numeric_limits<IStream::StreamSize>::max)() && "readline: buffer larger than a stream can address");
            return istream.readline(buffer, static_cast<IStream::StreamSize>(bufferCount));
        }

        template< typename Type_t >
        inline bool write(OStream& stream, const Type_t& value)
        {
            return stream.write(reinterpret_cast<const char*>(&value), sizeof(value)) == sizeof(value);
        }

        template< typename Type_t >
        inline bool write(OStream& stream)
        {
            const Type_t defaulted;
            return stream.write(reinterpret_cast<const char*>(&defaulted), sizeof(defaulted)) == sizeof(defaulted);
        }

        template<>
        inline bool write<void>(OStream& /*stream*/)
        {
            return true;
        }
#endif



        template< typename Type_t >
        constexpr size_t sizeOf() { return sizeof(Type_t); }

        template<>
        constexpr size_t sizeOf<void>() { return 0; }

        template< typename Type_t >
        constexpr void copyTo(char* buffer)
        { constexpr Type_t temp; std::memcpy(buffer, (const void*)&temp, sizeof(temp) ); }

        template< typename Type_t >
        constexpr void copyTo(char* buffer, const Type_t& value)
        { std::memcpy(buffer, (const void*)&value, sizeof(value)); }
    } // END: utility
} // END: sub0

namespace sub0
{
    // OStream/IStream type aliases (used by the IPC headers, sub0pub/ipc/)
#if SUB0PUB_STD
    typedef std::ostream OStream;
    typedef std::istream IStream;
#else
    typedef utility::OStream OStream;
    typedef utility::IStream IStream;
#endif
} // END: sub0

#endif // CROG_SUB0PUB_UTILITY_STREAMS_HPP
