/** Two Data types: marginal cost of a second message type */
#include "fp_common.hpp"

Source<MsgA> gSourceA;
Sink<MsgA> gSinkA;
Source<MsgB> gSourceB;
Sink<MsgB> gSinkB;

extern "C" void fp_publish_a(int v) noexcept { gSourceA.send(MsgA{v}); }
extern "C" void fp_publish_b(float v) noexcept { gSourceB.send(MsgB{v}); }
