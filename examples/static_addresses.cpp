/** A thermometer wired to a fixed display: the message type decides
 *
 * Use when: a message type's receivers are known when the application is composed, and each publication
 * should be a direct call with no subscription table behind it.
 * Demonstrates: sub0::StaticTo, written beside the message type, with Subscribe, Publish and publish() exactly
 * as they are written for the runtime broker.
 * Story: a thermometer publishes one reading. The reading's type names the display that receives it, so the
 * publication compiles to a call of that display's receive(); nothing is registered at run time.
 * Keep in mind: a listed receiver has static storage and is declared before the type that names it. The list
 * is the whole set of receivers: the display's Subscribe base is empty (no vtable, no registration), and a
 * subscriber the list does not name is never called, which a debug build reports. receive() carries no
 * `override`, because that base has nothing to override. Remove the sub0_config line and the same source runs
 * on the runtime broker.
 * Run: Sub0Pub_Example_static_addresses returns zero when the checks pass.
 */
#include "sub0pub/sub0pub.hpp"

class TemperatureDisplay;
extern TemperatureDisplay fixedTemperatureDisplay;

struct TemperatureReading
{
    int celsius;
    using sub0_config = sub0::config<sub0::StaticTo<&fixedTemperatureDisplay>>;
};

class TemperatureDisplay final : public sub0::Subscribe<TemperatureReading>
{
public:
    void receive(const TemperatureReading& reading) noexcept
    {
        lastCelsius = reading.celsius;
        ++readingsReceived;
    }

    int lastCelsius = 0;
    unsigned readingsReceived = 0;
};

struct TemperatureSensor final : sub0::Publish<TemperatureReading>
{
    void measure(int celsius) noexcept { sub0::publish(*this, TemperatureReading{celsius}); }
};

TemperatureDisplay fixedTemperatureDisplay;

bool publishToTheListedReceiver()
{
    TemperatureSensor thermometer;
    thermometer.measure(22);

    // The number of receivers is a constant of the program, so a mistake in the list can fail the build.
    static_assert(sub0::Publish<TemperatureReading>::receiverCount() == 1U);
    return fixedTemperatureDisplay.lastCelsius == 22 && fixedTemperatureDisplay.readingsReceived == 1;
}

int main()
{
    return publishToTheListedReceiver() ? 0 : 1;
}
