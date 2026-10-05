/** Footprint: 1 type, 1 publisher, 1 receiver, 1 publish site, as a v2 StaticWiring (compare fp_1type.cpp) */
#include "sub0pub/sub0pub.hpp"

struct MsgA { int value; };

struct SinkA { MsgA last{}; void receive(const MsgA& d) noexcept { last = d; } };

SinkA gSinkA;
using Bus = sub0::StaticWiring<&gSinkA>;

extern "C" void fp_publish_a(int v) noexcept { Bus::publish(MsgA{v}); }
