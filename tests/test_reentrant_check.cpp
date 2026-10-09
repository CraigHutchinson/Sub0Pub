/** Tests for SUB0PUB_REENTRANT_CHECK: detection of table changes during a direct-iteration dispatch
 *  (the default). A nested publish is supported; subscribing or unsubscribing
 *  the dispatched type from its own receive() needs Snapshot dispatch.
 *
 * The violation handler is overridden to count instead of abort. Types are unique
 * to this translation unit (anonymous namespace) so their Broker<T> instantiations
 * are not shared with other test files that use the default configuration.
 */
#define SUB0PUB_REENTRANT_CHECK true

namespace { int gViolations = 0; }
#define SUB0PUB_REENTRANT_VIOLATION(what) (void)(what), ++gViolations

#include <optional>

#include "doctest.h"
#include "sub0pub/sub0pub.hpp"

namespace {

constexpr bool cCheckActive = SUB0PUB_REENTRANT_CHECK;

struct ReMsg { int value; };
struct OtherMsg { int value; };

struct RePublisher : sub0::Publish<ReMsg>
{
    void send(int value) { sub0::publish(*this, ReMsg{value}); }
};

struct OtherPublisher : sub0::Publish<OtherMsg>
{
    void send(int value) { sub0::publish(*this, OtherMsg{value}); }
};

struct PlainSubscriber final : sub0::Subscribe<ReMsg>
{
    int received = 0;
    void receive(const ReMsg&) noexcept override { ++received; }
};

} // namespace

TEST_CASE("Reentrant check: normal publish is not a violation") {
    gViolations = 0;
    RePublisher pub;
    PlainSubscriber a, b;
    pub.send(1);
    CHECK(a.received == 1);
    CHECK(b.received == 1);
    CHECK(gViolations == 0);
}

TEST_CASE("Reentrant check: publishing the same type from receive() is supported (nested publish)") {
    gViolations = 0;
    struct Echo : sub0::Subscribe<ReMsg>
    {
        RePublisher* pub = nullptr;
        int received = 0;
        void receive(const ReMsg& m) noexcept override { ++received; if (m.value > 0) pub->send(m.value - 1); }
    };
    RePublisher pub;
    Echo echo;
    echo.pub = &pub;
    pub.send(1);
    CHECK(echo.received == 2); // 1, then nested 0: direct dispatch does not change the table
    CHECK(gViolations == 0);
}

TEST_CASE("Reentrant check: publishing a different type from receive() is allowed") {
    gViolations = 0;
    struct Relay : sub0::Subscribe<ReMsg>
    {
        OtherPublisher other;
        void receive(const ReMsg& m) noexcept override { other.send(m.value); }
    };
    struct OtherSub : sub0::Subscribe<OtherMsg>
    {
        int received = 0;
        void receive(const OtherMsg&) noexcept override { ++received; }
    };
    RePublisher pub;
    Relay relay;
    OtherSub sink;
    pub.send(4);
    CHECK(sink.received == 1);
    CHECK(gViolations == 0);
}

TEST_CASE("Reentrant check: subscribing and unsubscribing the same type from receive() is detected") {
    gViolations = 0;
    struct Mutator : sub0::Subscribe<ReMsg>
    {
        std::optional<PlainSubscriber> child;
        void receive(const ReMsg& m) noexcept override
        {
            if (m.value == 1) child.emplace(); // subscribe during dispatch
            else child.reset();                // unsubscribe during dispatch
        }
    };
    // In snapshot mode, destroying a snapshotted subscriber mid-dispatch is a use-after-free (issue #5)
    if (!cCheckActive)
        return;

    RePublisher pub;
    Mutator mutator;
    pub.send(1);
    CHECK(gViolations == 1);
    pub.send(2);
    CHECK(gViolations == 2);

    // Outside dispatch, table changes are not violations
    mutator.child.emplace();
    mutator.child.reset();
    CHECK(gViolations == 2);
}

namespace {
/// Default dispatch (DirectChecked in this TU), no publish context, with a domain and filter()
struct ClosingMsg { int value; using sub0_config = sub0::config<sub0::Scoped, sub0::Filter>; };

struct Closer final : sub0::Subscribe<ClosingMsg>
{
    sub0::Domain<ClosingMsg>& domain;
    bool destroy;
    int* received;
    Closer(sub0::Domain<ClosingMsg>& d, bool destroySelf, int* count) noexcept
        : sub0::Subscribe<ClosingMsg>(d), domain(d), destroy(destroySelf), received(count) {}
    bool filter(const ClosingMsg&) noexcept override
    {
        domain.close(); // a table change during its own dispatch
        if (destroy)
            delete this;
        return true;
    }
    void receive(const ClosingMsg&) noexcept override { ++*received; }
};

struct ClosingPub final : sub0::Publish<ClosingMsg>
{
    using Publish::Publish;
    void send(int v) noexcept { sub0::publish(*this, ClosingMsg{v}); }
};
} // namespace

TEST_CASE("Reentrant check: closing a domain from filter() under DirectChecked is detected and stops delivery") {
    for (const bool destroy : {false, true}) // destroying itself after closing: ASan-clean
    {
        CAPTURE(destroy);
        gViolations = 0;
        int received = 0;
        sub0::Domain<ClosingMsg> domain;
        ClosingPub pub(domain);
        auto* closer = new Closer(domain, destroy, &received);
        pub.send(1);
        CHECK(received == 0);
        CHECK(domain.isClosed());
        CHECK(gViolations == (cCheckActive ? 1 : 0));
        if (!destroy)
        {
            CHECK_FALSE(closer->isSubscribed());
            delete closer; // detached: destruction is not a table change
            CHECK(gViolations == (cCheckActive ? 1 : 0));
        }
    }
}
