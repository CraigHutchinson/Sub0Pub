/** Forward temperature telemetry without echoing incoming readings
 *
 * Use when: a locally composed wiring should send over a link and also accept incoming messages.
 * Demonstrates: Forward<TelemetryLink>, wire(), publish() and publishFrom().
 * Story: an outgoing reading reaches the display and link. An incoming reading names
 * that link as its origin, so it reaches the display without being sent back to the link.
 * Keep in mind: Forward ignores send results; the transport handles backpressure. Acceptance
 * is not remote delivery. Skipping the incoming link prevents immediate echo, not arbitrary
 * network cycles. The link here is a simulation.
 * Run: Sub0Pub_Example_link_forwarding returns zero when the checks pass.
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

bool forwardWithoutEcho()
{
    TelemetryLink link;
    TemperatureDisplay display;
    sub0::Forward<TelemetryLink> forward(link);
    auto wiring = sub0::wire(display, forward);

    wiring.publish(TemperatureReading{20}); // Outgoing: display and link.
    wiring.publishFrom(link, TemperatureReading{21}); // Incoming: display only.

    // Forward ignores send results. Use a Route when the publisher needs a rejection report.
    return display.readingsReceived == 2 && link.readingsAccepted == 1;
}

int main()
{
    return forwardWithoutEcho() ? 0 : 1;
}
