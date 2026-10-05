#include "doctest.h"
#include "sub0pub/sub0pub.hpp"

#include <string>

namespace {

struct IntFloatPublisher : sub0::Publish<int>, sub0::Publish<float> {
    void sendInt(int v) { sub0::publish(this, v); }
    void sendFloat(float v) { sub0::publish(this, v); }
};

struct MultiSub : sub0::SubscribeAll<int, float> {
    int intTotal = 0;
    float floatTotal = 0.0f;
    void receive(const int& v) noexcept override { intTotal += v; }
    void receive(const float& v) noexcept override { floatTotal += v; }
};

} // namespace

TEST_CASE("SubscribeAll receives multiple types") {
    IntFloatPublisher pub;
    MultiSub sub;

    pub.sendInt(10);
    pub.sendFloat(2.5f);

    CHECK(sub.intTotal == 10);
    CHECK(sub.floatTotal == doctest::Approx(2.5f));
}

TEST_CASE("SubscribeAll with tuple type list") {
    using Types = std::tuple<int, float>;

    struct TupleSub : sub0::SubscribeAll<Types> {
        int intVal = 0;
        float floatVal = 0.0f;
        void receive(const int& v) noexcept override { intVal = v; }
        void receive(const float& v) noexcept override { floatVal = v; }
    };

    IntFloatPublisher pub;
    TupleSub sub;

    pub.sendInt(7);
    pub.sendFloat(3.14f);

    CHECK(sub.intVal == 7);
    CHECK(sub.floatVal == doctest::Approx(3.14f));
}

TEST_CASE("SubscribeAll::Count is correct") {
    CHECK(sub0::SubscribeAll<int, float, double>::Count == 3);
    CHECK(sub0::SubscribeAll<std::tuple<int, float>>::Count == 2);
}
