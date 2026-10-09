// EXPECT: "needs a publish context": cancel() on a type without one names the option
#include "sub0pub/sub0pub.hpp"
struct Msg { int v; };
struct S : sub0::Subscribe<Msg> { void receive(const Msg&) noexcept override { cancel(); } };
int main() { S s; }
