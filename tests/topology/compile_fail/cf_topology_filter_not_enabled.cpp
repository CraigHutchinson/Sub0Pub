// EXPECT: declares filter\(\), but its Data type is not configured
#include "sub0pub/sub0pub.hpp"
class Display;
extern Display display;

struct Reading { int celsius; using sub0_config = sub0::config<sub0::StaticTo<&display>>; }; // no sub0::Filter
class Display final : public sub0::Subscribe<Reading>
{
public:
    bool filter(const Reading&) noexcept { return true; }
    void receive(const Reading&) noexcept {}
};
struct Source final : sub0::Publish<Reading> { void send() noexcept { sub0::publish(*this, Reading{1}); } };
Display display;
int main() { Source source; source.send(); }
