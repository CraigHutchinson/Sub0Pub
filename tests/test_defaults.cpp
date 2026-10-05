/** The default configuration is the cheapest correct dispatch; every costlier feature is opt-in and detected
 *
 * This translation unit sets no SUB0PUB_* policy macro, so types here use the Builtin defaults. The thread check is
 * forced on (it is the debug-build default) and counts violations instead of aborting.
 */
#define SUB0PUB_THREAD_CHECK true
#include <atomic>
namespace { std::atomic<int> gThreadViolations{0}; }
#define SUB0PUB_THREAD_VIOLATION(what) ((void)(what), gThreadViolations.fetch_add(1))

#include <optional>
#include <thread>

#include "doctest.h"
#include "sub0pub/sub0pub.hpp"

namespace {

struct DefMsg { int value; };
using DefConfig = sub0::config_t<DefMsg>;

static_assert(DefConfig::dispatch != sub0::Dispatch::Snapshot, "default: direct iteration (checked in debug builds)");
static_assert(DefConfig::context == sub0::Context::None, "default: no publish context (cancel() is opt-in)");
static_assert(!DefConfig::filter, "default: no filter() (opt-in)");
static_assert(std::is_same_v<DefConfig::Lock, sub0::NoLock>, "default: no lock (opt-in)");
static_assert(sizeof(sub0::Publish<DefMsg>) == 1, "a publisher is an empty handle");

struct DefPublisher : sub0::Publish<DefMsg>
{
    void send(int v) noexcept { sub0::publish(*this, DefMsg{v}); }
};

/// The first delivery on the first thread holds the dispatch open until the other thread has published
struct Holder : sub0::Subscribe<DefMsg>
{
    std::atomic<bool> inside{false};
    std::atomic<bool> release{false};
    std::atomic<int> received{0};
    void receive(const DefMsg& m) noexcept override
    {
        received.fetch_add(1);
        if (m.value == 1)
        {
            inside.store(true);
            while (!release.load())
                std::this_thread::yield();
        }
    }
};

} // namespace

TEST_CASE("Defaults: two threads using an unlocked type at once are reported (SUB0PUB_THREAD_CHECK)") {
    Holder holder;
    gThreadViolations = 0;
    std::thread first([&] { DefPublisher p; p.send(1); });
    while (!holder.inside.load())
        std::this_thread::yield();
    {
        DefPublisher p;
        p.send(2);                  // overlaps the first thread's dispatch of the same table
    }
    holder.release.store(true);
    first.join();
    CHECK(gThreadViolations.load() == 1);
    CHECK(holder.received.load() == 2);
}

TEST_CASE("Defaults: using an unlocked type from several threads in turn is not reported") {
    struct Counter : sub0::Subscribe<DefMsg> {
        std::atomic<int> received{0};
        void receive(const DefMsg&) noexcept override { received.fetch_add(1); }
    } counter;
    gThreadViolations = 0;
    for (int t = 0; t < 3; ++t)
    {
        std::thread worker([] { DefPublisher p; p.send(0); });
        worker.join();
    }
    CHECK(gThreadViolations.load() == 0);
    CHECK(counter.received.load() == 3);
}

namespace {
/// The migration recipe for callback-time subscription changes, on the Builtin default (no project header)
struct SnapRecipeMsg { int value; using sub0_config = sub0::config<sub0::Snapshot>; };
static_assert(sub0::config_t<SnapRecipeMsg>::dispatch == sub0::Dispatch::Snapshot, "");
static_assert(sub0::config_t<SnapRecipeMsg>::context == sub0::Context::ThreadLocal, "Snapshot selects a context");
static_assert(sub0::config<sub0::StaticContext, sub0::Snapshot>::context == sub0::Context::Static,
              "Snapshot keeps a context already chosen");

struct SnapRecipeSub final : sub0::Subscribe<SnapRecipeMsg>
{
    int received = 0;
    std::optional<SnapRecipeSub>* late = nullptr;
    void receive(const SnapRecipeMsg&) noexcept override
    {
        ++received;
        if (late && !*late)
            late->emplace(); // subscribe the same type from inside its own dispatch
    }
};
} // namespace

TEST_CASE("Defaults: config<Snapshot> alone allows subscribing from receive()") {
    struct Pub final : sub0::Publish<SnapRecipeMsg> { void send(int v) noexcept { sub0::publish(*this, SnapRecipeMsg{v}); } } pub;
    std::optional<SnapRecipeSub> late;
    SnapRecipeSub first;
    first.late = &late;
    gThreadViolations = 0;
    pub.send(1);
    CHECK(first.received == 1);
    REQUIRE(late);
    CHECK(late->received == 0); // added during the dispatch: called from the next publish
    pub.send(2);
    CHECK(late->received == 1);
    CHECK(gThreadViolations.load() == 0);
}
