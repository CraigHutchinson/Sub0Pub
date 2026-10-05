/** Footprint: 1 type, 1 publisher, 1 subscriber, 1 publish site, configured Lean (compare fp_1type.cpp) */
#include "sub0pub/sub0pub.hpp"

struct MsgA { int value; using sub0_config = sub0::config<sub0::Direct, sub0::NoContext, sub0::NoFilter>; };

struct SinkA final : sub0::Subscribe<MsgA> { MsgA last{}; void receive(const MsgA& d) noexcept override { last = d; } };
struct SourceA : sub0::Publish<MsgA> { void send(const MsgA& d) noexcept { sub0::publish(*this, d); } };

SourceA gSourceA;
SinkA gSinkA;

extern "C" void fp_publish_a(int v) noexcept { gSourceA.send(MsgA{v}); }
