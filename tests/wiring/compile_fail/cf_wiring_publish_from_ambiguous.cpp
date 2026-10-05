// EXPECT: "Origin must be bound exactly once": two links of one transport type cannot be told apart by type
#include "sub0pub/sub0pub.hpp"
struct Sample { int value; };
struct Radio { void send(const Sample&) noexcept {} };
int main()
{
    Radio a;
    Radio b;
    sub0::Forward<Radio> linkA(a);
    sub0::Forward<Radio> linkB(b);
    const auto bus = sub0::wire(linkA, linkB);
    bus.publishFrom<Radio>(Sample{1});
}
