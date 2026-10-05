/** One Data type used from several translation units shares one table and one configuration */
#include "doctest.h"
#include "app_types.hpp"

// Defined in test_multi_tu_b.cpp
void publishImuFromB();
int subscribersOfImuSeenByB();
extern int gMismatches;

namespace {
struct ImuSink : sub0::Subscribe<Imu> {
    int received = 0;
    void receive(const Imu&) noexcept override { ++received; }
};
}

TEST_CASE("config: subscriber in TU a receives publish from TU b, configuration agrees") {
    ImuSink sink;
    CHECK(sink.isSubscribed());
    publishImuFromB();
    CHECK(sink.received == 1);
    CHECK(subscribersOfImuSeenByB() == 1);
    CHECK(gMismatches == 0);
}
