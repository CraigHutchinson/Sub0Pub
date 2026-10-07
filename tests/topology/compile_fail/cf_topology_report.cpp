// EXPECT: no matching|cannot convert|no viable|too many arguments|C2660|C2664
#include "sub0pub/sub0pub.hpp"
class Display;
extern Display display;

struct Reading { int celsius; using sub0_config = sub0::config<sub0::ThreadLocalContext, sub0::StaticTo<&display>>; };
class Display final : public sub0::Subscribe<Reading>
{
public:
    void receive(const Reading&) noexcept {}
};
struct Source final : sub0::Publish<Reading>
{
    void send() noexcept { sub0::PublishReport report; sub0::publish(*this, Reading{1}, report); } // routes need the broker
};
Display display;
int main() { Source source; source.send(); }
