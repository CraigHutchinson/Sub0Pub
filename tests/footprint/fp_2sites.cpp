/** One Data type published from two call sites: marginal cost of another publish call */
#include "fp_common.hpp"

Source<MsgA> gSourceA;
Sink<MsgA> gSinkA;

extern "C" void fp_publish_a(int v) noexcept { gSourceA.send(MsgA{v}); }
extern "C" void fp_publish_a2(int v) noexcept { gSourceA.send(MsgA{v + 1}); }
