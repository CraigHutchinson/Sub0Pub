/** A dynamic subscription bound to a concrete member
 *
 * Use when: a receiver should keep a normal member function while subscribing through the runtime broker.
 * Demonstrates: run-time and compile-time member binding spellings as member-owned subscriptions.
 * Story: a display receives one reading through its subscription and retains the concrete handler's state.
 * Keep in mind: the spike adapter still bridges through the broker's virtual receive(); it does not change dispatch.
 * Run: each api_convergence_*member target returns zero when its member receives the reading.
 */
#include "member_subscription.hpp"

#ifndef SUB0PUB_SPIKE_BINDING
#error "Build through this directory's CMake project to select a member-binding spike"
#endif

struct Reading
{
    int celsius;
};

class Display
{
public:
    Display() noexcept
#if SUB0PUB_SPIKE_BINDING == 0
        : subscription_(*this, &Display::receive)
#else
        : subscription_(*this)
#endif
    {}

    void receive(const Reading& reading) noexcept
    {
        lastCelsius_ = reading.celsius;
        ++count_;
    }

    int lastCelsius() const noexcept { return lastCelsius_; }
    unsigned count() const noexcept { return count_; }

private:
#if SUB0PUB_SPIKE_BINDING == 0
    sub0::spike::MemberSubscription<Reading, Display> subscription_;
#else
    sub0::spike::StaticMemberSubscription<Reading, Display, &Display::receive> subscription_;
#endif
    int lastCelsius_ = 0;
    unsigned count_ = 0;
};

struct Thermometer final : sub0::Publish<Reading>
{
    void measure(int celsius) noexcept { sub0::publish(*this, Reading{celsius}); }
};

int main()
{
    Display display;
    Thermometer thermometer;
    thermometer.measure(22);
    return display.count() == 1U && display.lastCelsius() == 22 ? 0 : 1;
}
