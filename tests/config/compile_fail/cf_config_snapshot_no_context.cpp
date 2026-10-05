// EXPECT: "Snapshot needs a publish context"
// Without a dispatch frame, a subscriber disconnected (or destroyed) during a Snapshot dispatch cannot be removed from
// that dispatch's snapshot: a receiver deleting a later subscriber left a dangling pointer (ASan: heap-use-after-free).
#include "sub0pub/sub0pub.hpp"
struct Msg { using sub0_config = sub0::config<sub0::Snapshot, sub0::NoContext>; };
struct S : sub0::Subscribe<Msg> { void receive(const Msg&) noexcept override {} };
int main() { S s; }
