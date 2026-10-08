/** Tests for the no-receivers policy: a publication that reaches no receiver is a reported failure, unless its
 *  Data type says an absent receiver is expected (sub0::AllowNoReceivers).
 *
 * The check is switched on for this translation unit's default (as a debug build has it) and its action counts
 * instead of aborting. Types are unique to this translation unit (anonymous namespace).
 */
#define SUB0PUB_NO_RECEIVERS_CHECK true
namespace { int gUnheard = 0; }
#define SUB0PUB_NO_RECEIVERS(what) ((void)(what), ++gUnheard)

#include <mutex>

#include "doctest.h"
#include "sub0pub/sub0pub.hpp"

namespace {

template<class Data>
struct Source final : sub0::Publish<Data>
{
    void send(int value) noexcept { sub0::publish(*this, Data{value}); }
};

template<class Data>
struct Sink final : sub0::Subscribe<Data>
{
    Sink() noexcept { this->trySubscribe(); } // a locked type does not register in its base constructor
    ~Sink() { this->disconnect(); }
    void receive(const Data& data) noexcept override { total += data.value; }
    int total = 0;
};

struct Plain { int value; };                                                                        // the default
struct Snapshotted { int value; using sub0_config = sub0::config<sub0::Snapshot>; };
struct Optional { int value; using sub0_config = sub0::config<sub0::AllowNoReceivers>; };
struct Always { int value; using sub0_config = sub0::config<sub0::ReportNoReceivers>; };

struct Mutex
{
    void lock() noexcept { m.lock(); }
    void unlock() noexcept { m.unlock(); }
    std::mutex m;
};
struct Locked { int value; using sub0_config = sub0::config<sub0::LockWith<Mutex>>; };

struct Session { int value; using sub0_config = sub0::config<sub0::Scoped>; };

static_assert(sub0::config_t<Plain>::noReceivers == sub0::NoReceivers::Report, "SUB0PUB_NO_RECEIVERS_CHECK sets the default");
static_assert(sub0::config_t<Optional>::noReceivers == sub0::NoReceivers::Allow);
static_assert(sub0::config_t<Always>::noReceivers == sub0::NoReceivers::Report);

// StaticFirst: a bound receiver takes the message, so an empty table is not an unheard publication
class Panel;
extern Panel panel;
struct Status { int value; using sub0_config = sub0::config<sub0::StaticFirst<&panel>>; };
class Panel final : public sub0::Subscribe<Status>
{
public:
    void receive(const Status& status) noexcept { total += status.value; }
    int total = 0;
};
Panel panel;

template<class Data>
void checkReported()
{
    Source<Data> source;
    const int before = gUnheard;

    source.send(1);
    CHECK(gUnheard == before + 1);
    CHECK(sub0::receiverCount<Data>(source) == 0U);

    {
        Sink<Data> sink;
        CHECK(sub0::receiverCount<Data>(source) == 1U);
        source.send(2);
        CHECK(sink.total == 2);
        CHECK(gUnheard == before + 1); // heard: not reported
    }

    source.send(3); // the receiver has gone again
    CHECK(gUnheard == before + 2);
}

} // namespace

TEST_CASE("no receivers: reported for direct dispatch") { checkReported<Plain>(); }
TEST_CASE("no receivers: reported for snapshot dispatch") { checkReported<Snapshotted>(); }
TEST_CASE("no receivers: reported for a locked type") { checkReported<Locked>(); }
TEST_CASE("no receivers: reported for a type that asks for it in every build") { checkReported<Always>(); }

TEST_CASE("no receivers: a type that allows it is never reported") {
    Source<Optional> source;
    const int before = gUnheard;
    source.send(1);
    CHECK(gUnheard == before);
    CHECK(sub0::receiverCount<Optional>(source) == 0U); // the call site can still ask, and decide for itself
}

TEST_CASE("no receivers: a bound receiver of a StaticFirst type counts as a receiver") {
    Source<Status> source;
    const int before = gUnheard;
    source.send(5);
    CHECK(panel.total == 5);
    CHECK(gUnheard == before);
    CHECK(sub0::receiverCount<Status>(source) == 1U);
}

TEST_CASE("no receivers: a closed Domain drops the publication without reporting it") {
    struct SessionSource final : sub0::Publish<Session>
    {
        explicit SessionSource(sub0::Domain<Session>& domain) noexcept : sub0::Publish<Session>(domain) {}
        void send(int value) noexcept { sub0::publish(*this, Session{value}); }
    };
    sub0::Domain<Session> domain;
    {
        SessionSource source(domain);
        const int before = gUnheard;
        source.send(1); // open, and nobody subscribed: reported
        CHECK(gUnheard == before + 1);
        domain.close();
        source.send(2); // closed: the session has ended, which is not an unheard publication
        CHECK(gUnheard == before + 1);
    }
}
