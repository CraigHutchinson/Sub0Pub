/** Deliberate build-contract violation: this unit wires `long long` to a receiver from outside the type, and
 *  mismatch_b.cpp does not see that configuration. Exit code 0 means both reports were made: the debug check sees
 *  the two units disagree, and the stray unit's publication, into a table nobody subscribed to, is not silent.
 */
namespace mismatch { class Sink; extern Sink sink; }
extern int gMismatches;
extern int gUnheard;
#define SUB0PUB_CONFIG_MISMATCH(what) ((void)(what), ++gMismatches)
#define SUB0PUB_NO_RECEIVERS(what) ((void)(what), ++gUnheard)

#include "sub0pub/sub0pub.hpp"

SUB0PUB_CONFIGURE(long long, sub0::StaticTo<&mismatch::sink>);

int gMismatches = 0;
int gUnheard = 0;
void publishWithoutConfiguration(long long value); // mismatch_b.cpp

namespace mismatch
{
    class Sink final : public sub0::Subscribe<long long>
    {
    public:
        void receive(const long long& value) noexcept { total += value; }
        long long total = 0;
    };
    Sink sink;

    struct Source final : sub0::Publish<long long>
    {
        void send(long long value) noexcept { sub0::publish(*this, value); }
    };
}

int main()
{
    mismatch::Source source;
    source.send(20);                 // a direct call
    publishWithoutConfiguration(21); // brokered by a unit that missed the configuration: nobody receives it
    return (mismatch::sink.total == 20 && gMismatches == 1 && gUnheard == 1) ? 0 : 1;
}
