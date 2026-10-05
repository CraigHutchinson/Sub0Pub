// EXPECT: a subscriber declaring filter() on a type without the filter option does not compile, even without
// `override` (it would otherwise be silently ignored). gcc: "conflicting return type"; clang: "different return
// type" / "overrides a 'final' function"; MSVC: C2555 "return type differs".
#include "sub0pub/sub0pub.hpp"
struct Msg { int v; };
struct S : sub0::Subscribe<Msg>
{
    void receive(const Msg&) noexcept override {}
    bool filter(const Msg& m) noexcept { return m.v > 0; }
};
int main() { S s; }
