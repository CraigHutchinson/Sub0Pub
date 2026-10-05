/** One Data type with a second subscriber: marginal cost of another subscriber of an existing type */
#include "fp_common.hpp"

Source<MsgA> gSourceA;
Sink<MsgA> gSinkA;
Sink<MsgA> gSinkA2;

extern "C" void fp_publish_a(int v) noexcept { gSourceA.send(MsgA{v}); }
