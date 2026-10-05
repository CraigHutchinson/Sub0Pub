// EXPECT: "cancel() needs a publish context"
#include "sub0pub/sub0pub.hpp"
struct Msg { using sub0_config = sub0::config<sub0::Direct, sub0::NoContext>; };
struct S : sub0::Subscribe<Msg> { void receive(const Msg&) noexcept override { cancel(); } };
int main() { S s; }
