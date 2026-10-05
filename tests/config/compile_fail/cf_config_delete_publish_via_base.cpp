// EXPECT: "protected" (or MSVC "inaccessible"): a publisher is destroyed as its own type, never through Publish<T>*
#include "sub0pub/sub0pub.hpp"
struct Msg { int v; };
struct P : sub0::Publish<Msg> {};
int main()
{
    sub0::Publish<Msg>* base = new P;
    delete base; // Publish<Msg>::~Publish() is protected and non-virtual
}
