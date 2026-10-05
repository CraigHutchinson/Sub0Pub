/** Configuration resolution, per-type policies, scoped domains */
#include "doctest.h"
#include "app_types.hpp"

#include <optional>
#include <type_traits>

namespace {

template<class Data>
struct Sink final : sub0::Subscribe<Data> {
    using sub0::Subscribe<Data>::Subscribe; // Scoped types construct from a Domain
    int received = 0;
    void receive(const Data&) noexcept override { ++received; }
};

template<class Data>
struct Source : sub0::Publish<Data> {
    using sub0::Publish<Data>::Publish;
    void send(const Data& d) noexcept { sub0::publish(*this, d); }
};

template<class Data, class = void> struct has_filter : std::false_type {};
// A usable filter() returns bool; without the filter option only a never-called void guard exists
template<class Data> struct has_filter<Data, std::enable_if_t<std::is_same_v<bool,
    decltype(std::declval<sub0::Subscribe<Data>&>().filter(std::declval<const Data&>()))>>> : std::true_type {};

} // namespace

// --- Resolution: each mechanism binds the intended configuration ---
static_assert(sub0::config_t<Imu>::capacity == 2, "member alias");
static_assert(sub0::config_t<gps::Fix>::capacity == 3, "ADL declaration");
static_assert(sub0::config_t<gps::Mode>::capacity == 1, "ADL declaration for an enum");
static_assert(sub0::config_t<int>::capacity == 5, "traits specialisation");
static_assert(sub0::config_t<CoreTempC>::capacity == 6, "tagged type configured through its tag");
static_assert(sub0::config_t<Plain>::capacity == 4, "project default from SUB0PUB_CONFIG_HEADER");
static_assert(sub0::config_t<double>::capacity == 4, "unconfigured fundamental type uses the project default");

// Per-type options layer on the project default, not on Builtin
static_assert(sub0::config_t<Lean>::capacity == 4 && sub0::config_t<Lean>::dispatch == sub0::Dispatch::Direct, "");

// Policy-shaped interface: filter() exists only where configured
static_assert(has_filter<Imu>::value, "");
static_assert(!has_filter<gps::Fix>::value, "NoFilter removes the filter() virtual");
static_assert(!has_filter<Lean>::value, "");

// Publishers carry no vtable (no virtual destructor) and only an empty broker handle
static_assert(!std::is_polymorphic_v<sub0::Publish<Imu>>, "");
static_assert(sizeof(sub0::Publish<Imu>) == 1, "global-storage publisher is an empty handle");

TEST_CASE("config: per-type capacity is enforced independently") {
    Sink<Imu> a, b, overflow;              // Imu capacity 2
    CHECK(a.isSubscribed());
    CHECK(b.isSubscribed());
    CHECK_FALSE(overflow.isSubscribed());

    Sink<Plain> p[4];                      // Plain capacity 4 (project default)
    for (auto& s : p) CHECK(s.isSubscribed());
    Sink<Plain> plainOverflow;
    CHECK_FALSE(plainOverflow.isSubscribed());

    Source<Imu> src;
    src.send(Imu{});
    CHECK(a.received == 1);
    CHECK(b.received == 1);
    CHECK(overflow.received == 0);
}

TEST_CASE("config: trySubscribe() reclaims a freed slot") {
    std::optional<Sink<Imu>> a{std::in_place};
    Sink<Imu> b, late;
    CHECK_FALSE(late.isSubscribed());
    a.reset();
    CHECK(late.trySubscribe() == sub0::SubscribeResult::Subscribed);
}

TEST_CASE("config: lean configuration dispatches without context or filter") {
    Sink<Lean> a, b;
    Source<Lean> src;
    src.send(Lean{1});
    CHECK(a.received == 1);
    CHECK(b.received == 1);
}

TEST_CASE("config: cancel() stops dispatch under a context policy") {
    struct Canceller : sub0::Subscribe<int> {
        void receive(const int&) noexcept override { cancel(); }
    };
    Canceller first;
    Sink<int> second;
    Source<int> src;
    src.send(1);
    CHECK(second.received == 0);
}

TEST_CASE("config: filter() is honoured where configured") {
    struct EvenOnly : sub0::Subscribe<int> {
        int received = 0;
        bool filter(const int& v) noexcept override { return (v % 2) == 0; }
        void receive(const int&) noexcept override { ++received; }
    };
    EvenOnly s;
    Source<int> src;
    src.send(1);
    src.send(2);
    CHECK(s.received == 1);
}

TEST_CASE("config: scoped domains isolate the same Data type (issue #5)") {
    sub0::Domain<Session> sessionA, sessionB;
    Sink<Session> subA(sessionA), subB(sessionB);
    Source<Session> pubA(sessionA);
    pubA.send(Session{1});
    CHECK(subA.received == 1);
    CHECK(subB.received == 0);   // no cross-talk
}

// Registry fingerprints effective values, not type names
namespace {
struct NamedA : sub0::with<sub0::Builtin, sub0::Capacity<3>> {};
struct NamedB : sub0::with<sub0::Builtin, sub0::Capacity<3>> {};
struct NamedC : sub0::with<sub0::Builtin, sub0::Capacity<4>> {};
}
static_assert(sub0::detail::configFingerprint<NamedA>() == sub0::detail::configFingerprint<NamedB>(), "same values, different names");
static_assert(sub0::detail::configFingerprint<NamedA>() != sub0::detail::configFingerprint<NamedC>(), "different values");

extern int gViolations;

TEST_CASE("config: cancel() from another domain's subscriber does not cancel this domain's dispatch") {
    sub0::Domain<Session> a, b;
    Sink<Session> subB(b);

    struct CancelOther : sub0::Subscribe<Session> {
        using sub0::Subscribe<Session>::Subscribe;
        sub0::Subscribe<Session>* other = nullptr;
        int received = 0;
        void receive(const Session&) noexcept override { ++received; other->cancel(); }
    };
    CancelOther first(a);
    first.other = &subB;
    Sink<Session> second(a);

    Source<Session> pubA(a);
    pubA.send(Session{1});
    CHECK(first.received == 1);
    CHECK(second.received == 1); // was skipped before: "A first=1 second=0 B=0"
    CHECK(subB.received == 0);
}

TEST_CASE("config: cancel() still stops its own domain's dispatch") {
    sub0::Domain<Session> a;
    struct CancelOwn : sub0::Subscribe<Session> {
        using sub0::Subscribe<Session>::Subscribe;
        void receive(const Session&) noexcept override { cancel(); }
    };
    CancelOwn first(a);
    Sink<Session> second(a);
    Source<Session> pub(a);
    pub.send(Session{1});
    CHECK(second.received == 0);
}

TEST_CASE("config: DirectChecked reports a table change during its own dispatch, per domain") {
    sub0::Domain<SessionChecked> a, b;
    Source<SessionChecked> pubA(a);

    struct Joiner : sub0::Subscribe<SessionChecked> {
        using sub0::Subscribe<SessionChecked>::Subscribe;
        sub0::Domain<SessionChecked>* join = nullptr;
        std::optional<Sink<SessionChecked>> late;
        void receive(const SessionChecked&) noexcept override { if (!late) late.emplace(*join); }
    };
    Joiner joiner(a);

    gViolations = 0;
    joiner.join = &b;                // A's dispatch subscribes into B: independent table, allowed
    pubA.send(SessionChecked{1});
    CHECK(gViolations == 0);

    joiner.late.reset();
    joiner.join = &a;                // A's dispatch subscribes into A: its own table, reported
    pubA.send(SessionChecked{1});
    CHECK(gViolations == 1);
    joiner.late.reset();
}

TEST_CASE("config: DirectChecked allows a nested publish (the table does not change)") {
    sub0::Domain<SessionChecked> a;
    Sink<SessionChecked> sink(a);
    Source<SessionChecked> pubA(a);
    struct Echo : sub0::Subscribe<SessionChecked> {
        using sub0::Subscribe<SessionChecked>::Subscribe;
        Source<SessionChecked>* target = nullptr;
        void receive(const SessionChecked& m) noexcept override { if (m.value > 0) target->send(SessionChecked{m.value - 1}); }
    };
    Echo echo(a);
    echo.target = &pubA;
    gViolations = 0;
    pubA.send(SessionChecked{1});
    CHECK(gViolations == 0);
    CHECK(sink.received == 2);
}

TEST_CASE("config: tagged payloads are distinct channels configured by tag") {
    Sink<CoreTempC> t;
    Source<CoreTempC> src;
    src.send(CoreTempC{42.0f});
    CHECK(t.received == 1);
}
