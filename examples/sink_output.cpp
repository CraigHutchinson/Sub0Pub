/** A thermometer with a fixed output type — hiding the receiver list
 *
 * Use when: a publisher should not expose the concrete wiring type in its interface.
 * Demonstrates: Sink<TemperatureReading> wrapping wire(), used as the Publisher output type.
 * Story: a display is wired locally; the thermometer receives a Sink instead of the
 * receiver-list type. Its reading still reaches that display.
 * Keep in mind: Sink is non-owning: wiring and receivers must outlive it. It introduces a
 * type-erased call boundary; it does not own receivers or provide thread safety.
 * Run: Sub0Pub_Example_sink_output returns zero when the checks pass.
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

bool hideTheWiringType()
{
    TemperatureDisplay display;
    auto wiring = sub0::wire(display);

    // A Sink gives the publisher one fixed output type, regardless of the receiver list.
    // The wiring and its receivers must outlive this non-owning output.
    TemperatureSensor<sub0::Sink<TemperatureReading>> thermometer{sub0::Sink<TemperatureReading>{wiring}};
    thermometer.measure(21);

    return display.lastCelsius == 21 && display.readingsReceived == 1;
}

int main()
{
    return hideTheWiringType() ? 0 : 1;
}
