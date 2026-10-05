/** Sub0Pub: DefaultSerialisation: the default IPC frame protocol
 * @remark Part of Sub0Pub (https://github.com/BareCpper/Sub0Pub), MIT License: see LICENSE.md.
 *         Included by the umbrella header <sub0pub/sub0pub.hpp>.
 */
#ifndef CROG_SUB0PUB_IPC_PROTOCOL_HPP
#define CROG_SUB0PUB_IPC_PROTOCOL_HPP

#include "sub0pub/ipc/binary_writer.hpp"
#include "sub0pub/ipc/binary_reader.hpp"
#include "sub0pub/utility/hash.hpp"
#include "sub0pub/utility/type_info.hpp"
#include <cstdint>

namespace sub0
{
    /** Binary protocol for serialised signal and data transfer
     * @remark The protocol consists of a Header chunk followed by Header::dataBytes bytes of payload data
     */
    struct DefaultSerialisation
    {
        struct Prefix
        {
            const uint32_t magic = sub0::utility::FourCC<'S', 'U', 'B', '0'>::value; //< Magic number to identify Sub0 network protocol packets
        };

        /** Header containing signal type information
        */
        struct Header
        {
            uint32_t typeId; ///< Data type identifier @note The Id may be user specified for inter-process
            uint32_t dataBytes; ///< Count of bytes that follow after the header data

            Header() = default;

            /** header for specified Data type
            */
            template<typename Data>
            Header( const Data& /*data*/ )
#if SUB0PUB_TYPEIDNAME
                : typeId(detail::TypeInfo<Data>::typeId())
#else
                : typeId(utility::typeHash<Data>())
#endif
                , dataBytes(sizeof(Data))
            {}

            /** Sort by typeId only
            */
            bool operator < (const Header& rhs) const
            { return typeId < rhs.typeId; }

            /** Compare full equality 
            */
            bool operator == (const Header& rhs) const
            { return (typeId == rhs.typeId) && (dataBytes == rhs.dataBytes); }
        };

        struct Postfix
        {
            uint8_t delim = '\n';
            bool operator==(const Postfix& rhs) const { return delim == rhs.delim; }
        };

        using Writer = BinaryWriter<Prefix, Header, Postfix>;
        using Reader = BinaryReader<Prefix, Header, Postfix>;
    };
} // END: sub0

#endif // CROG_SUB0PUB_IPC_PROTOCOL_HPP
