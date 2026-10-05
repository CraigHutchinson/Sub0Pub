#include "sub0pub/sub0pub.hpp" // missing SUB0PUB_CONFIGURE(long, ...): resolves to the project default

namespace {
struct LongSink : sub0::Subscribe<long> { void receive(const long&) noexcept override {} };
}

void subscribeLongWithoutConfiguration()
{
    LongSink unconfigured;
}
