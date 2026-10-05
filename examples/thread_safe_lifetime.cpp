/** Two clocks share one counter — publishing from multiple threads
 *
 * Use when: concurrent publishers need a receiver whose registration respects its object lifetime.
 * Demonstrates: LockWith<std::mutex>, explicit trySubscribe()/disconnect(), and atomic receiver state.
 * Story: TickCounter registers after its fields are constructed. Two threads each create a Clock
 * and publish 100 ticks. After both threads join, the counter must hold 200; its destructor
 * then disconnects before its derived state is destroyed.
 * Keep in mind: broker locking protects the subscription table, not application callback state;
 * callbacks can overlap, hence the atomic counter. disconnect() is the teardown pattern for
 * in-flight callbacks, but this particular run joins publishers before destruction.
 * Run: Sub0Pub_Example_thread_safe_lifetime returns zero when all 200 ticks were received.
 */
#include "sub0pub/broker.hpp"
#include <atomic>
#include <mutex>
#include <thread>

struct Tick
{
    using sub0_config = sub0::config<sub0::LockWith<std::mutex>>;
};

struct Clock final : sub0::Publish<Tick>
{
    void tick() noexcept { sub0::publish(*this, Tick{}); }
};

struct TickCounter final : sub0::Subscribe<Tick>
{
    std::atomic<unsigned> ticksReceived{0};

    TickCounter() noexcept
    {
        // Locked subscribers do not auto-register: all derived state must be ready first.
        trySubscribe();
    }

    ~TickCounter()
    {
        disconnect(); // Wait for in-flight callbacks before destroying derived state.
    }

    void receive(const Tick&) noexcept override
    {
        ++ticksReceived; // Broker locking protects its table; callbacks can run concurrently.
    }
};

int main()
{
    TickCounter counter;
    if (!counter.isSubscribed())
        return 1;

    const auto publishTicks = []
    {
        Clock clock;
        for (unsigned i = 0; i < 100; ++i)
            clock.tick();
    };

    std::thread firstClock(publishTicks);
    std::thread secondClock(publishTicks);
    firstClock.join();
    secondClock.join();

    return counter.ticksReceived.load() == 200 ? 0 : 2;
}
