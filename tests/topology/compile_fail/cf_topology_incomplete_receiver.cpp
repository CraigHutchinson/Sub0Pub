// EXPECT: incomplete type|undefined type|C2027
#include "sub0pub/sub0pub.hpp"
class Display;
extern Display display;

struct Reading { int celsius; using sub0_config = sub0::config<sub0::StaticTo<&display>>; };
// The publisher's translation unit never sees the definition of Display
struct Source final : sub0::Publish<Reading> { void send() noexcept { sub0::publish(*this, Reading{1}); } };
int main() { Source source; source.send(); }
