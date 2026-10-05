/** Deliberate ODR hazard: TU a configures `long` via traits, TU b forgot to include that configuration.
 * The debug registry must report it at the second registration. Exit code 0 means it was detected.
 */
#include "sub0pub/sub0pub.hpp"

SUB0PUB_CONFIGURE(long, sub0::Capacity<3>);

int gMismatches = 0;
int gViolations = 0; // counted by SUB0PUB_REENTRANT_VIOLATION
void subscribeLongWithoutConfiguration(); // mismatch_b.cpp

namespace {
struct LongSink : sub0::Subscribe<long> { void receive(const long&) noexcept override {} };
}

int main()
{
    LongSink configured;
    subscribeLongWithoutConfiguration();
    return gMismatches == 1 ? 0 : 1;
}
