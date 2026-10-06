/** A fixed wiring owned by its containing object
 *
 * Use when: receiver objects and their publisher are composed into one longer-lived object.
 * Demonstrates: a Wiring member initialized after the receivers, then copied into a typed Publisher.
 * Story: the station owns its display and audit log; its thermometer routes readings through its wiring member.
 * Keep in mind: receivers must outlive all uses of the wiring, so keep the declaration and initialization order.
 * Run: both api_convergence_member_wiring_* targets return zero when both receivers observe the reading.
 */
#include "sub0pub/wiring.hpp"

#include <utility>

#ifndef SUB0PUB_SPIKE_WIRING_TYPE
#error "Build through this directory's CMake project to select a wiring-type spike"
#endif

struct Reading
{
    int celsius;
};

struct Display
{
    void receive(const Reading& reading) noexcept { lastCelsius = reading.celsius; }
    int lastCelsius = 0;
};

struct Audit
{
    void receive(const Reading& reading) noexcept { lastCelsius = reading.celsius; }
    int lastCelsius = 0;
};

template<class Output>
struct Thermometer final : sub0::Publisher<Thermometer<Output>, Output>
{
    using sub0::Publisher<Thermometer<Output>, Output>::Publisher;
    void measure(int celsius) noexcept { this->publish(Reading{celsius}); }
};

struct Station
{
    Display display;
    Audit audit;
#if SUB0PUB_SPIKE_WIRING_TYPE == 0
    using Wiring = sub0::Wiring<Display, Audit>;
#elif SUB0PUB_SPIKE_WIRING_TYPE == 1
    using Wiring = decltype(sub0::wire(std::declval<Display&>(), std::declval<Audit&>()));
#else
#error "SUB0PUB_SPIKE_WIRING_TYPE must be 0 (explicit) or 1 (deduced)"
#endif
    Wiring wiring;
    Thermometer<Wiring> thermometer;

    Station() noexcept : wiring(display, audit), thermometer(wiring) {}
};

int main()
{
    Station station;
    station.thermometer.measure(23);
    return station.display.lastCelsius == 23 && station.audit.lastCelsius == 23 ? 0 : 1;
}
