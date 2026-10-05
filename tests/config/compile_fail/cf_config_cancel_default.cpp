// EXPECT: "SUB0PUB_CANCEL": cancel() on a type without a publish context names the opt-in
#include "sub0pub/sub0pub.hpp"
struct Msg { int v; };
struct S : sub0::Subscribe<Msg> { void receive(const Msg&) noexcept override { cancel(); } };
int main() { S s; }
