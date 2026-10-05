// EXPECT: filter() marked override but NoFilter removed it from the interface
#include "sub0pub/sub0pub.hpp"
struct Msg { using sub0_config = sub0::config<sub0::NoFilter>; };
struct S : sub0::Subscribe<Msg> {
    void receive(const Msg&) noexcept override {}
    bool filter(const Msg&) noexcept override { return true; }
};
int main() { S s; }
