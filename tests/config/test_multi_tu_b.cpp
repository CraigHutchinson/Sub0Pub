#include "app_types.hpp"

int gMismatches = 0; // counted by SUB0PUB_CONFIG_MISMATCH (see CMakeLists.txt)
int gViolations = 0; // counted by SUB0PUB_REENTRANT_VIOLATION

namespace {
struct ImuSource : sub0::Publish<Imu> {
    void send(const Imu& d) noexcept { sub0::publish(*this, d); }
};
struct Probe : sub0::Subscribe<Imu> {
    void receive(const Imu&) noexcept override {}
};
}

void publishImuFromB()
{
    ImuSource src;
    src.send(Imu{});
}

/// Capacity 2: exactly one slot is left if TU a holds one subscriber
int subscribersOfImuSeenByB()
{
    Probe a;
    Probe b;
    return (a.isSubscribed() && !b.isSubscribed()) ? 1 : -1;
}
