/** Forward through a link whose address is fixed in the wiring type
 *
 * Use when: the local receiver and transport both have static storage and fixed identities.
 * Demonstrates: StaticForward<&link>, StaticWiring and origin-aware publishFrom().
 * Story: the fixed display and link receive an outgoing reading. An incoming reading
 * from that same link reaches only the display, avoiding an immediate echo.
 * Keep in mind: the objects must be initialized before publication. Forwarding ignores
 * send results and does not guarantee remote delivery or prevent arbitrary network cycles.
 * The link is a simulation; see route_reports.cpp when send rejection must be reported.
 * Run: Sub0Pub_Example_static_link_forwarding returns zero when the checks pass.
 */
#include "sub0pub/wiring.hpp"

struct TemperatureReading { int celsius; };

struct TelemetryLink
{
    unsigned readingsAccepted = 0;

    void send(const TemperatureReading&) noexcept
    {
        ++readingsAccepted; // A real link copies/serializes the reading before returning.
    }
};

struct TemperatureDisplay
{
    unsigned readingsReceived = 0;
    void receive(const TemperatureReading&) noexcept { ++readingsReceived; }
};

TelemetryLink fixedLink;
TemperatureDisplay fixedDisplay;
sub0::StaticForward<&fixedLink> fixedForward;
using FixedWiring = sub0::StaticWiring<&fixedDisplay, &fixedForward>;

bool forwardThroughFixedAddresses()
{
    FixedWiring::publish(TemperatureReading{20});
    FixedWiring::publishFrom(fixedLink, TemperatureReading{21});
    return fixedDisplay.readingsReceived == 2 && fixedLink.readingsAccepted == 1;
}

int main()
{
    return forwardThroughFixedAddresses() ? 0 : 1;
}
