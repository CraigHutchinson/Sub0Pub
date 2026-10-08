// EXPECT: declares a filter\(\) the runtime broker would not call
#include "sub0pub/sub0pub.hpp"
class Display;
extern Display display;

struct Reading { int celsius; using sub0_config = sub0::config<sub0::Filter, sub0::StaticTo<&display>>; };
class Display final : public sub0::Subscribe<Reading>
{
public:
    bool filter(const Reading&) const noexcept { return true; } // const: hides the broker's virtual, never overrides it
    void receive(const Reading&) noexcept {}
};
struct Source final : sub0::Publish<Reading> { void send() noexcept { sub0::publish(*this, Reading{1}); } };
Display display;
int main() { Source source; source.send(); }
