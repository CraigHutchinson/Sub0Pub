/** An increment source updates a running total — the smallest runtime pattern
 *
 * Use when: you want a compact starting point for one publisher and one subscriber.
 * Demonstrates: Publish<uint32_t>, Subscribe<uint32_t>, and a noexcept receive() callback.
 * Story: RunningTotal subscribes when constructed. IncrementSource publishes 3141 once,
 * and RunningTotal adds that value to its own total. Scope exit unsubscribes the subscriber.
 * Keep in mind: this is the default unlocked runtime broker; nothing here is wired at compile time. The subscription
 * must succeed before delivery is expected; the sample checks that its one receiver subscribed.
 * Run: Sub0Pub_MinimalExample returns zero when the total is 3141; it prints nothing.
 */
#include "sub0pub/broker.hpp"
#include <cstdint>

class IncrementSource final : public sub0::Publish<uint32_t>
{
public:
    void add(uint32_t amount) noexcept { sub0::publish(*this, amount); }
};

class RunningTotal final : public sub0::Subscribe<uint32_t>
{
public:
    uint32_t total = 0;
    void receive(const uint32_t& amount) noexcept { total += amount; }
};

int main()
{
    IncrementSource source;
    RunningTotal counter;
    if (!counter.isSubscribed())
        return 1;

    source.add(3141U);
    return counter.total == 3141U ? 0 : 2;
}
