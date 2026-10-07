// EXPECT: would reach nobody
#include "sub0pub/sub0pub.hpp"
struct Reading { int celsius; using sub0_config = sub0::config<sub0::StaticTo<>>; }; // nobody, and not AllowNoReceivers
struct Source final : sub0::Publish<Reading> { void send() noexcept { sub0::publish(*this, Reading{1}); } };
int main() { Source source; source.send(); }
