/** Footprint: 2 types, each with 1 publisher, 1 subscriber, 1 publish site (Full; the marginal cost of a Data type) */
#include "fp_axis.hpp"

struct MsgA { int value; using sub0_config = sub0::with<fp::Full>; };
struct MsgB { int value; using sub0_config = sub0::with<fp::Full>; };

struct SinkA : sub0::Subscribe<MsgA> { MsgA last{}; void receive(const MsgA& d) noexcept override { last = d; } };
struct SourceA : sub0::Publish<MsgA> { void send(const MsgA& d) noexcept { sub0::publish(*this, d); } };
struct SinkB : sub0::Subscribe<MsgB> { MsgB last{}; void receive(const MsgB& d) noexcept override { last = d; } };
struct SourceB : sub0::Publish<MsgB> { void send(const MsgB& d) noexcept { sub0::publish(*this, d); } };

SourceA gSourceA;
SinkA gSinkA;
SourceB gSourceB;
SinkB gSinkB;

extern "C" void fp_publish_a(int v) noexcept { gSourceA.send(MsgA{v}); }
extern "C" void fp_publish_b(int v) noexcept { gSourceB.send(MsgB{v}); }

extern "C" { char fp_sizeof_Subscribe[sizeof(sub0::Subscribe<MsgA>)]; char fp_sizeof_Publish[sizeof(sub0::Publish<MsgA>)]; }
