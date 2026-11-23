#include "doctest.h"
#include "sub0pub/sub0pub.hpp"

#include <string>
#include <tuple>
#include <type_traits>

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

TEST_CASE("SubscribeAll expands a Types list in place") {
    using Numbers = sub0::Types<int, float>;

    struct ListSub : sub0::SubscribeAll<Numbers> {
        int intVal = 0;
        float floatVal = 0.0f;
        void receive(const int& v) noexcept override { intVal = v; }
        void receive(const float& v) noexcept override { floatVal = v; }
    };

    IntFloatPublisher pub;
    ListSub sub;

    pub.sendInt(7);
    pub.sendFloat(3.14f);

    CHECK(sub.intVal == 7);
    CHECK(sub.floatVal == doctest::Approx(3.14f));
    static_assert(std::is_base_of_v<sub0::Subscribe<int>, ListSub> && std::is_base_of_v<sub0::Subscribe<float>, ListSub>);
}

TEST_CASE("SubscribeAll combines lists, nested lists and plain types") {
    using Inner = sub0::Types<float>;
    using Outer = sub0::Types<int, Inner>;
    static_assert(sub0::SubscribeAll<Outer, double>::Count == 3);
    static_assert(std::is_base_of_v<sub0::SubscribeAll<int, float, double>, sub0::SubscribeAll<Outer, double>>);
}

namespace {
/// A std::tuple is a message type like any other
using Pair = std::tuple<int, float>;
struct PairPublisher : sub0::Publish<Pair> {
    void send(int i, float f) noexcept { sub0::publish(*this, Pair{i, f}); }
};
} // namespace

TEST_CASE("A std::tuple is one message type, for Subscribe and SubscribeAll alike") {
    struct PairSub : sub0::Subscribe<Pair> {
        Pair last{};
        void receive(const Pair& p) noexcept override { last = p; }
    };
    struct PairAndInt : sub0::SubscribeAll<Pair, int> {
        int pairs = 0;
        int ints = 0;
        void receive(const Pair&) noexcept override { ++pairs; }
        void receive(const int&) noexcept override { ++ints; }
    };
    static_assert(sub0::SubscribeAll<Pair, int>::Count == 2);

    PairSub one;
    PairAndInt both;
    PairPublisher pub;
    IntFloatPublisher numbers;
    pub.send(1, 2.5f);
    numbers.sendInt(3);

    CHECK(std::get<0>(one.last) == 1);
    CHECK(std::get<1>(one.last) == doctest::Approx(2.5f));
    CHECK(both.pairs == 1);
    CHECK(both.ints == 1);
}

TEST_CASE("SubscribeAll::Count is correct") {
    CHECK(sub0::SubscribeAll<int, float, double>::Count == 3);
    CHECK(sub0::SubscribeAll<sub0::Types<int, float>>::Count == 2);
    CHECK(sub0::SubscribeAll<std::tuple<int, float>>::Count == 1);
}
