extern int gMismatches;
extern int gUnheard;
#define SUB0PUB_CONFIG_MISMATCH(what) ((void)(what), ++gMismatches)
#define SUB0PUB_NO_RECEIVERS(what) ((void)(what), ++gUnheard)

#include "sub0pub/sub0pub.hpp" // missing SUB0PUB_CONFIGURE(long long, ...): for this unit the type is brokered

namespace
{
    struct Source final : sub0::Publish<long long>
    {
        void send(long long value) noexcept { sub0::publish(*this, value); }
    };
}

void publishWithoutConfiguration(long long value)
{
    Source source;
    source.send(value);
}
