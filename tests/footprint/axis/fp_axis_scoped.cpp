/** Footprint: 1 type, 1 publisher, 1 subscriber, 1 publish site (Full, Storage=Scoped) */
#include "fp_axis.hpp"

struct MsgA { int value; using sub0_config = sub0::with<fp::Full, sub0::Scoped>; };

struct SinkA : sub0::Subscribe<MsgA> { using Subscribe::Subscribe; MsgA last{}; void receive(const MsgA& d) noexcept override { last = d; } };
struct SourceA : sub0::Publish<MsgA> { using Publish::Publish; void send(const MsgA& d) noexcept { sub0::publish(*this, d); } };

sub0::Domain<MsgA> gDomain;
SourceA gSourceA(gDomain);
SinkA gSinkA(gDomain);

extern "C" void fp_publish_a(int v) noexcept { gSourceA.send(MsgA{v}); }

extern "C" { char fp_sizeof_Subscribe[sizeof(sub0::Subscribe<MsgA>)]; char fp_sizeof_Publish[sizeof(sub0::Publish<MsgA>)]; char fp_sizeof_Domain[sizeof(sub0::Domain<MsgA>)]; }
