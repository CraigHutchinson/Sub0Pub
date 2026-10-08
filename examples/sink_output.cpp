/** A thermometer with a fixed output type: hiding the receiver list
 *
 * Use when: a publisher must not name its wiring: it is compiled into a library, or sits behind an interface
 * that cannot be a template.
 * Demonstrates: Sink<TemperatureReading> wrapping wire(), held by an ordinary (non-template) publisher class.
 * Story: a display is wired locally; the thermometer is given a Sink instead of the receiver-list type. Its
 * reading still reaches that display.
 * Keep in mind: Sink is non-owning: wiring and receivers must outlive it. It introduces a type-erased call
 * boundary (one indirect call per publication); it does not own receivers or provide thread safety. A publisher
 * in the application's own source does not need one: it can derive from Publish<T> and let the message type
 * decide the delivery.
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

class TemperatureSensor final
{
public:
    explicit TemperatureSensor(sub0::Sink<TemperatureReading> output) noexcept : output_(output) {}
    void measure(int celsius) noexcept { output_.publish(TemperatureReading{celsius}); }

private:
    sub0::Sink<TemperatureReading> output_;
};

bool hideTheWiringType()
{
    TemperatureDisplay display;
    auto wiring = sub0::wire(display);

    // A Sink gives the publisher one fixed output type, regardless of the receiver list.
    // The wiring and its receivers must outlive this non-owning output.
    TemperatureSensor thermometer{sub0::Sink<TemperatureReading>{wiring}};
    thermometer.measure(21);

    return display.lastCelsius == 21 && display.readingsReceived == 1;
}

int main()
{
    return hideTheWiringType() ? 0 : 1;
}
