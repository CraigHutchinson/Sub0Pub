/** A thermometer with fixed receiver addresses — StaticWiring
 *
 * Use when: receivers have static storage and their addresses can be part of the wiring type.
 * Demonstrates: StaticWiring<&receiver> and the Publisher mixin with an empty wiring object.
 * Story: a thermometer publishes one reading through FixedWiring to the fixed display.
 * No runtime receiver list is passed to the thermometer.
 * Keep in mind: the receiver must have static storage and be initialized before publication.
 * This does not support adding receivers dynamically; use a port when that is required.
 * Run: Sub0Pub_Example_static_addresses returns zero when the checks pass.
 */
#include "sub0pub/wiring.hpp"

struct TemperatureReading { int celsius; };

struct TemperatureDisplay
{
    int lastCelsius = 0;
    unsigned readingsReceived = 0;

    void receive(const TemperatureReading& reading) noexcept
    {
        lastCelsius = reading.celsius;
        ++readingsReceived;
    }
};

template<class Output>
struct TemperatureSensor final : sub0::Publisher<TemperatureSensor<Output>, Output>
{
    using sub0::Publisher<TemperatureSensor<Output>, Output>::Publisher;
    void measure(int celsius) noexcept { this->publish(TemperatureReading{celsius}); }
};

TemperatureDisplay fixedTemperatureDisplay;
using FixedWiring = sub0::StaticWiring<&fixedTemperatureDisplay>;

bool bindFixedAddresses()
{
    TemperatureSensor<FixedWiring> thermometer{FixedWiring{}};
    thermometer.measure(22);
    return fixedTemperatureDisplay.lastCelsius == 22 && fixedTemperatureDisplay.readingsReceived == 1;
}

int main()
{
    return bindFixedAddresses() ? 0 : 1;
}
