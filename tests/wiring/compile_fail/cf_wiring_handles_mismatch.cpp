// EXPECT: the opt-in K14 guard rejects a receiver whose receive() does not accept the message
#include "sub0pub/sub0pub.hpp"
struct Sample { int value; };
struct Other { int value; };
struct Logger { void receive(const Other&) noexcept {} };
static_assert(sub0::handles_v<Logger, Sample>, "Logger must receive Sample");
int main() {}
