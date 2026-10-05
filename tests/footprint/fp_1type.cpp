/** One Data type: one publisher, one subscriber, one publish call site */
#include "fp_common.hpp"

Source<MsgA> gSourceA;
Sink<MsgA> gSinkA;

extern "C" void fp_publish_a(int v) noexcept { gSourceA.send(MsgA{v}); }

SUB0PUB_FP_SIZEOF(Subscribe, sub0::Subscribe<MsgA>)
SUB0PUB_FP_SIZEOF(Publish, sub0::Publish<MsgA>)
