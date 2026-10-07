// EXPECT: derives from Subscribe<Data> but has no receive
#include "sub0pub/sub0pub.hpp"
class Display;
extern Display display;

struct Reading { int celsius; using sub0_config = sub0::config<sub0::StaticTo<&display>>; };
class Display final : public sub0::Subscribe<Reading>
{
public:
    void receive(Reading&) noexcept {} // not const: capability routing would skip it without a diagnostic
};
struct Source final : sub0::Publish<Reading> { void send() noexcept { sub0::publish(*this, Reading{1}); } };
Display display;
int main() { Source source; source.send(); }
