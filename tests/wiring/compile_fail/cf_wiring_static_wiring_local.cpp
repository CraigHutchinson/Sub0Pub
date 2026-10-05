// EXPECT: a StaticWiring binds objects with static storage duration only (known issue K20: no runtime-added
// or automatic-storage receiver in B2; use wire(...) for dynamic lifetimes)
#include "sub0pub/sub0pub.hpp"
struct Sample { int value; };
struct Controller { void receive(const Sample&) noexcept {} };
int main()
{
    Controller local;
    sub0::StaticWiring<&local>::publish(Sample{1});
}
