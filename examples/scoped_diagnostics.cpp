/** One-shot diagnostics leave a cooling controller running
 *
 * Use when: runtime observers behind fixed wiring need broker policy and session ownership.
 * Demonstrates: BrokerPort, Scoped Domain, Snapshot and callback disconnect().
 * Story: two one-shot probes disconnect after their first reading. The fixed cooling
 * controller keeps receiving; closing the diagnostic session also leaves that fixed path intact.
 * The session's readings are configured with AllowNoReceivers: probes are optional observers, so a
 * reading that no probe is listening for is expected, not the failure it would otherwise be reported as.
 * Keep in mind: Snapshot permits callback self-removal. The Domain must outlive its bound
 * handles. This unlocked example is not safe for concurrent access.
 * Run: Sub0Pub_Example_scoped_diagnostics returns zero when the checks pass.
 */
#include "sub0pub/wiring.hpp"
#include "sub0pub/wiring/broker_port.hpp"

struct SessionTemperatureReading
{
    int celsius;
    using sub0_config = sub0::config<sub0::Scoped, sub0::Snapshot, sub0::AllowNoReceivers>;
};

struct SessionCoolingController
{
    unsigned readingsReceived = 0;
    bool fanRunning = false;

    void receive(const SessionTemperatureReading& reading) noexcept
    {
        fanRunning = reading.celsius >= 21;
        ++readingsReceived;
    }
};

struct OneShotProbe final : sub0::Subscribe<SessionTemperatureReading>
{
    using Subscribe::Subscribe;
    unsigned readingsReceived = 0;

    void receive(const SessionTemperatureReading&) noexcept
    {
        ++readingsReceived;
        disconnect(); // BrokerPort with Snapshot permits removal during a callback.
    }
};

bool letProbesDisconnectThemselves()
{
    sub0::Domain<SessionTemperatureReading> diagnosticSession;
    SessionCoolingController controller;
    sub0::BrokerPort<SessionTemperatureReading> diagnostics(diagnosticSession);
    auto wiring = sub0::wire(controller, diagnostics);

    OneShotProbe firstProbe(diagnosticSession);
    OneShotProbe secondProbe(diagnosticSession);
    wiring.publish(SessionTemperatureReading{20});
    if (firstProbe.readingsReceived != 1 || secondProbe.readingsReceived != 1)
        return false;

    wiring.publish(SessionTemperatureReading{21}); // Both probes have disconnected; the controller remains wired.
    if (controller.readingsReceived != 2 || !controller.fanRunning)
        return false;
    diagnosticSession.close();
    wiring.publish(SessionTemperatureReading{22}); // Closing the diagnostic session leaves the fixed path intact.

    return controller.readingsReceived == 3 && controller.fanRunning &&
        firstProbe.readingsReceived == 1 && secondProbe.readingsReceived == 1;
}

int main()
{
    return letProbesDisconnectThemselves() ? 0 : 1;
}
