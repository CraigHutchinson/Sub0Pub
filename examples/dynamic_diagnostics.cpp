/** Subscribe an optional diagnostic probe beside a cooling controller
 *
 * Use when: one receiver of a message type is fixed and known, and others subscribe and unsubscribe at run time.
 * Demonstrates: sub0::StaticFirst beside the message type: the listed controller is called directly, then the
 * runtime broker delivers to whoever is subscribed. Capacity bounds the runtime side, and isSubscribed()
 * reports a probe that did not fit.
 * Story: the controller first runs alone. A probe subscribes for one reading; a second probe cannot fit. The
 * first unsubscribes, and the controller continues without it.
 * Keep in mind: the listed receiver has static storage and is called first. The runtime side is the ordinary
 * broker with the type's configuration, so its rules apply: with the default dispatch a probe must not subscribe or
 * unsubscribe from inside its own receive() (see scoped_diagnostics.cpp for that). StaticFirst keeps the broker's cost
 * on every publication; where the set of receivers is closed, list them all with StaticTo instead.
 * Run: Sub0Pub_Example_dynamic_diagnostics returns zero when the checks pass.
 */
#include "sub0pub/sub0pub.hpp"

class CoolingController;
extern CoolingController coolingController;

struct TemperatureReading
{
    int celsius;
    using sub0_config = sub0::config<sub0::Capacity<1>, sub0::StaticFirst<&coolingController>>;
};

class CoolingController final : public sub0::Subscribe<TemperatureReading>
{
public:
    void receive(const TemperatureReading& reading) noexcept
    {
        fanRunning = reading.celsius >= 21;
        ++readingsReceived;
    }

    unsigned readingsReceived = 0;
    bool fanRunning = false;
};

struct DiagnosticProbe final : sub0::Subscribe<TemperatureReading>
{
    unsigned readingsReceived = 0;
    void receive(const TemperatureReading&) noexcept { ++readingsReceived; }
};

struct TemperatureSensor final : sub0::Publish<TemperatureReading>
{
    void measure(int celsius) noexcept { sub0::publish(*this, TemperatureReading{celsius}); }
};

CoolingController coolingController;

bool attachAndRemoveAProbe()
{
    TemperatureSensor thermometer;

    thermometer.measure(20); // The controller works with no diagnostic receiver: it is not an unheard publication.
    if (coolingController.readingsReceived != 1)
        return false;

    {
        DiagnosticProbe probe;
        DiagnosticProbe waitingProbe; // The runtime side holds one subscriber: this one is not subscribed.
        if (!probe.isSubscribed() || waitingProbe.isSubscribed())
            return false;

        thermometer.measure(21);
        if (coolingController.readingsReceived != 2 || probe.readingsReceived != 1 || waitingProbe.readingsReceived != 0)
            return false;
    } // Both probes unsubscribe as their lifetimes end.

    thermometer.measure(22);
    return coolingController.readingsReceived == 3 && coolingController.fanRunning;
}

int main()
{
    return attachAndRemoveAProbe() ? 0 : 1;
}
