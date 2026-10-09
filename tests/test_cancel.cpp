#include "doctest.h"
#include "sub0pub/sub0pub.hpp"

namespace {

// cancel() needs a publish context: the type opts in
struct Tick { int value; using sub0_config = sub0::config<sub0::ThreadLocalContext>; };

struct CancelPublisher : sub0::Publish<Tick> {
    void send(int value) { sub0::publish(this, Tick{value}); }
};

struct CancelAfterN : sub0::Subscribe<Tick> {
    int callCount = 0;
    int cancelAfter;
    CancelAfterN(int n) : cancelAfter(n) {}
    void receive(const Tick&) noexcept override {
        ++callCount;
        if (callCount >= cancelAfter)
            cancel();
    }
};

struct Counter : sub0::Subscribe<Tick> {
    int callCount = 0;
    void receive(const Tick&) noexcept override { ++callCount; }
};

} // namespace

TEST_CASE("Cancel stops remaining subscribers") {
    CancelPublisher pub;
    CancelAfterN canceller(1); // Cancel on first receive
    Counter after;             // Should not receive

    pub.send(42);
    CHECK(canceller.callCount == 1);
    CHECK(after.callCount == 0);
}

TEST_CASE("Cancel does not affect subsequent publishes") {
    CancelPublisher pub;
    CancelAfterN canceller(1);
    Counter after;

    pub.send(1); // Cancelled
    CHECK(after.callCount == 0);

    // Reset canceller threshold so it won't cancel again
    canceller.cancelAfter = 999;

    pub.send(2); // Should go through to both
    CHECK(canceller.callCount == 2);
    CHECK(after.callCount == 1);
}

TEST_CASE("sub0::cancel<Data>(publisher) stops the publication from inside receive()") {
    struct Gate : sub0::Subscribe<Tick> {
        CancelPublisher* pub = nullptr;
        int callCount = 0;
        void receive(const Tick&) noexcept override { ++callCount; sub0::cancel<Tick>(*pub); }
    };
    CancelPublisher pub;
    Gate gate;
    gate.pub = &pub;
    Counter after;
    pub.send(1);
    CHECK(gate.callCount == 1);
    CHECK(after.callCount == 0);
}
