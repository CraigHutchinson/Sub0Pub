// EXPECT: "protected" (or MSVC "inaccessible"): a subscriber is destroyed as its own type, never through Subscribe<T>*
#include "sub0pub/sub0pub.hpp"
struct Msg { int v; };
struct S : sub0::Subscribe<Msg> { void receive(const Msg&) noexcept override {} };
int main()
{
    sub0::Subscribe<Msg>* base = new S;
    delete base; // Subscribe<Msg>::~Subscribe() is protected and non-virtual
}
