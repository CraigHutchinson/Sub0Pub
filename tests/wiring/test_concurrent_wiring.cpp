/** Shared static wiring needs stable bindings and thread-safe receivers. Run these tests under TSan. */
#include "doctest.h"
#include "sub0pub/sub0pub.hpp"
#include <array>
#include <atomic>
#include <cstdint>
#include <thread>

namespace {
struct ConcurrentSample { uint32_t value; bool stop; };
struct ConcurrentCounter
{
    std::atomic<uint32_t> calls{0}, sum{0};
    void receive(const ConcurrentSample& message) noexcept
    {
        calls.fetch_add(1, std::memory_order_relaxed);
        sum.fetch_add(message.value, std::memory_order_relaxed);
    }
    void reset() noexcept { calls.store(0); sum.store(0); }
};
struct ConcurrentGate
{
    bool receive(const ConcurrentSample& message) noexcept { return !message.stop; }
};
ConcurrentGate staticGate;
ConcurrentCounter staticBefore, staticAfter;
using ConcurrentBus = sub0::StaticWiring<&staticBefore, &staticGate, &staticAfter>;
constexpr uint32_t cThreadCount = 4, cPublications = 2000;

template<class Publish>
void publishConcurrently(Publish publish)
{
    std::atomic<uint32_t> ready{0};
    std::array<std::thread, cThreadCount> threads;
    for (uint32_t id = 0; id < cThreadCount; ++id)
        threads[id] = std::thread([&, id] {
            ready.fetch_add(1, std::memory_order_acq_rel);
            while (ready.load(std::memory_order_acquire) != cThreadCount)
                std::this_thread::yield();
            for (uint32_t i = 0; i < cPublications; ++i)
                publish(ConcurrentSample{id + 1, id % 2 == 0});
        });
    for (auto& thread : threads)
        thread.join();
}
void checkAll(const ConcurrentCounter& receiver)
{
    CHECK(receiver.calls.load() == cThreadCount * cPublications);
    CHECK(receiver.sum.load() == 10 * cPublications);
}
void checkUncancelled(const ConcurrentCounter& receiver)
{
    CHECK(receiver.calls.load() == 2 * cPublications);
    CHECK(receiver.sum.load() == 6 * cPublications); // values 2 and 4 continue
}
TEST_CASE("concurrent static wiring: B1 and B3 share immutable bindings")
{
    ConcurrentCounter before, after;
    ConcurrentGate gate;
    const auto bus = sub0::wire(before, gate, after);
    publishConcurrently([&](const auto& message) { bus.publish(message); });
    checkAll(before); checkAll(after);
    before.reset(); after.reset();
    const sub0::Sink<ConcurrentSample> sink(bus);
    publishConcurrently([&](const auto& message) { sink.publish(message); });
    checkAll(before); checkAll(after);
    before.reset(); after.reset();
    publishConcurrently([&](const auto& message) { bus.publishCancelable(message); });
    checkAll(before); checkUncancelled(after);
}
TEST_CASE("concurrent static wiring: B2 cancellation stays local to each publication")
{
    staticBefore.reset(); staticAfter.reset();
    publishConcurrently([](const auto& message) { ConcurrentBus::publish(message); });
    checkAll(staticBefore); checkAll(staticAfter);
    staticBefore.reset(); staticAfter.reset();
    publishConcurrently([](const auto& message) { ConcurrentBus::publishCancelable(message); });
    checkAll(staticBefore); checkUncancelled(staticAfter);
}
} // namespace
