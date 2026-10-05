// EXPECT: a Scoped type's subscriber cannot be constructed without a Domain
#include "sub0pub/sub0pub.hpp"
struct Msg { using sub0_config = sub0::config<sub0::Scoped>; };
struct S : sub0::Subscribe<Msg> { void receive(const Msg&) noexcept override {} };
int main() { S s; }
