/** One Data type forwarded to a StreamSerializer (IPC transmit side) */
#include "fp_common.hpp"

class Serializer : public sub0::StreamSerializer<>
                 , public sub0::ForwardSubscribe<MsgA, Serializer> {
public:
    explicit Serializer(sub0::OStream& out) : sub0::StreamSerializer<>(out) {}
};

extern sub0::OStream& fpOutput();

Source<MsgA> gSourceA;
Serializer gSerializer(fpOutput());

extern "C" void fp_publish_a(int v) noexcept { gSourceA.send(MsgA{v}); }
