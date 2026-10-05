// EXPECT: "configured in exactly one place"
#include "sub0pub/sub0pub.hpp"
struct Twice { using sub0_config = sub0::config<sub0::Capacity<2>>; };
SUB0PUB_CONFIGURE(Twice, sub0::Capacity<3>);
struct S : sub0::Subscribe<Twice> { void receive(const Twice&) noexcept override {} };
int main() { S s; }
