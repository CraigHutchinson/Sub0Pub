/** Reject an out-of-range temperature before displaying it
 *
 * Use when: an earlier receiver should stop delivery to later receivers in fixed wiring.
 * Demonstrates: an ordered wire() and a bool receive() result used by publishCancelable().
 * Story: TemperatureRangeCheck is bound before the display. It rejects 200 degrees,
 * so the display sees nothing; a later valid reading of 23 reaches the display.
 * Keep in mind: false stops this publication only. Ordinary publish() ignores the bool
 * result. This is fixed-wiring cancellation, not the runtime broker cancel() API.
 * Run: Sub0Pub_Example_static_cancellation returns zero when the checks pass.
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

struct TemperatureRangeCheck
{
    bool receive(const TemperatureReading& reading) noexcept { return reading.celsius >= -40 && reading.celsius <= 125; }
};

bool stopAnOutOfRangeReading()
{
    TemperatureRangeCheck rangeCheck;
    TemperatureDisplay display;
    auto wiring = sub0::wire(rangeCheck, display);

    // publishCancelable stops on false; ordinary publish ignores a receiver's return value.
    wiring.publishCancelable(TemperatureReading{200});
    if (display.readingsReceived != 0)
        return false;

    wiring.publishCancelable(TemperatureReading{23});
    return display.lastCelsius == 23 && display.readingsReceived == 1;
}

int main()
{
    return stopAnOutOfRangeReading() ? 0 : 1;
}
