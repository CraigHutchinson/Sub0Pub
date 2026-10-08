// EXPECT: no member named|is not a member|has no member
#include "sub0pub/sub0pub.hpp"
class Display;
extern Display display;

struct Reading { int celsius; using sub0_config = sub0::config<sub0::StaticTo<&display>>; };
class Display final : public sub0::Subscribe<Reading>
{
public:
    void receive(const Reading&) noexcept {}
};
Display display;
int main() { display.unsubscribe(); } // a statically wired receiver has no subscription to end
