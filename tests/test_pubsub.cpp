#include "doctest.h"
#include "sub0pub/sub0pub.hpp"

#include <vector>

namespace {

// Simple accumulator subscriber
struct Accumulator : sub0::Subscribe<int> {
    int total = 0;
    int callCount = 0;
    void receive(const int& value) noexcept override {
        total += value;
        ++callCount;
    }
};

// Simple publisher
struct IntPublisher : sub0::Publish<int> {
    void send(int value) { sub0::publish(this, value); }
};

// Multi-type publisher
struct MultiPublisher : sub0::Publish<int>, sub0::Publish<float> {
    void sendInt(int v) { sub0::publish(this, v); }
    void sendFloat(float v) { sub0::publish(this, v); }
};

struct FloatAccumulator : sub0::Subscribe<float> {
    float total = 0.0f;
    void receive(const float& value) noexcept override { total += value; }
};

// final: deleted through its own type; Subscribe<T> has no virtual destructor (MIGRATION.md)
struct OrderTracker final : sub0::Subscribe<int> {
    int id;
    static inline std::vector<int> receiveOrder;
    OrderTracker(int id) : id(id) {}
    void receive(const int&) noexcept override {
        receiveOrder.push_back(id);
    }
};

} // namespace

TEST_CASE("Basic publish-subscribe") {
    IntPublisher pub;
    Accumulator sub;

    pub.send(10);
    CHECK(sub.total == 10);
    CHECK(sub.callCount == 1);

    pub.send(20);
    CHECK(sub.total == 30);
    CHECK(sub.callCount == 2);
}

TEST_CASE("Multiple subscribers receive same message") {
    IntPublisher pub;
    Accumulator sub1;
    Accumulator sub2;
    Accumulator sub3;

    pub.send(5);
    CHECK(sub1.total == 5);
    CHECK(sub2.total == 5);
    CHECK(sub3.total == 5);
}

TEST_CASE("Subscriber destroyed mid-lifetime") {
    IntPublisher pub;
    Accumulator sub1;

    {
        Accumulator sub2;
        pub.send(10);
        CHECK(sub2.total == 10);
    }
    // sub2 destroyed, sub1 should still work
    pub.send(20);
    CHECK(sub1.total == 30); // 10 + 20
}

TEST_CASE("Subscription order preserved on removal") {
    IntPublisher pub;
    OrderTracker::receiveOrder.clear();

    auto* a = new OrderTracker(1);
    auto* b = new OrderTracker(2);
    auto* c = new OrderTracker(3);

    pub.send(0);
    CHECK(OrderTracker::receiveOrder == std::vector<int>{1, 2, 3});

    // Remove middle subscriber
    OrderTracker::receiveOrder.clear();
    delete b;

    pub.send(0);
    CHECK(OrderTracker::receiveOrder == std::vector<int>{1, 3});

    delete a;
    delete c;
}

TEST_CASE("Multi-type publish routes correctly") {
    MultiPublisher pub;
    Accumulator intSub;
    FloatAccumulator floatSub;

    pub.sendInt(42);
    CHECK(intSub.total == 42);
    CHECK(floatSub.total == 0.0f);

    pub.sendFloat(1.5f);
    CHECK(intSub.total == 42);
    CHECK(floatSub.total == doctest::Approx(1.5f));
}

TEST_CASE("No subscribers - a type that allows it publishes without effect") {
    // An absent receiver is a reported failure by default (test_no_receivers.cpp); a type opts out
    struct Unheard { int value; using sub0_config = sub0::config<sub0::AllowNoReceivers>; };
    struct UnheardPublisher : sub0::Publish<Unheard> {
        void send(int value) { sub0::publish(*this, Unheard{value}); }
    };
    UnheardPublisher pub;
    pub.send(99); // Should not crash, and is not reported
    CHECK(sub0::receiverCount<Unheard>(pub) == 0U);
}

TEST_CASE("Filter support") {
    // filter() is opt-in: the type enables it
    struct Reading { int value; using sub0_config = sub0::config<sub0::Filter>; };
    struct EvenOnly : sub0::Subscribe<Reading> {
        int total = 0;
        void receive(const Reading& r) noexcept override { total += r.value; }
        bool filter(const Reading& r) noexcept override { return (r.value % 2) == 0; }
    };
    struct ReadingPublisher : sub0::Publish<Reading> {
        void send(int value) noexcept { sub0::publish(*this, Reading{value}); }
    };

    ReadingPublisher pub;
    EvenOnly sub;

    pub.send(1);
    pub.send(2);
    pub.send(3);
    pub.send(4);

    CHECK(sub.total == 6); // 2 + 4
}
