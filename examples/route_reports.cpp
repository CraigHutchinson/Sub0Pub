/** Report a full telemetry link while preserving local delivery
 *
 * Use when: a runtime publisher needs transport acceptance/rejection feedback and an ingress path.
 * Demonstrates: Scoped Domain, Route, PublishReport and inject(), with ThreadLocalContext.
 * Story: a first reading is accepted by the link. The simulated queue then becomes full:
 * the next send is reported rejected, but the local display still receives it. Once the queue
 * drains, an injected incoming reading reaches the display without being echoed out through that route.
 * Keep in mind: a Route uses a subscriber slot; check that it subscribed. The Domain and transport
 * must outlive their bound handles. Acceptance is not remote delivery; immediate echo
 * suppression does not prevent arbitrary network cycles.
 * Run: Sub0Pub_Example_route_reports returns zero when the checks pass.
 */
#include "sub0pub/broker.hpp"

struct TemperatureReading
{
    int celsius;
    // Routes need a publish context to track the incoming link and report send results.
    using sub0_config = sub0::config<sub0::Scoped, sub0::ThreadLocalContext>;
};

struct TelemetryLink
{
    unsigned readingsAccepted = 0;
    bool queueFull = false;

    sub0::SendResult send(const TemperatureReading&) noexcept
    {
        if (queueFull)
            return sub0::SendResult::Full;

        ++readingsAccepted; // A real link copies/serializes here; acceptance is not remote delivery.
        return sub0::SendResult::Accepted;
    }
};

struct SubscribedTemperatureDisplay final : sub0::Subscribe<TemperatureReading>
{
    using Subscribe::Subscribe;
    unsigned readingsReceived = 0;
    void receive(const TemperatureReading&) noexcept { ++readingsReceived; }
};

struct TemperatureSensor final : sub0::Publish<TemperatureReading>
{
    using Publish::Publish;
};

bool reportWhenTheLinkIsFull()
{
    TelemetryLink link;
    sub0::Domain<TemperatureReading> session;
    SubscribedTemperatureDisplay display(session);
    TemperatureSensor thermometer(session);
    sub0::Route<TemperatureReading, TelemetryLink> route(session, link);
    if (!route.isSubscribed()) // A route uses a subscriber slot too.
        return false;

    sub0::PublishReport acceptedReport;
    sub0::publish(thermometer, TemperatureReading{20}, acceptedReport);
    if (acceptedReport.accepted != 1 || display.readingsReceived != 1)
        return false;

    link.queueFull = true;
    sub0::PublishReport rejectedReport;
    sub0::publish(thermometer, TemperatureReading{21}, rejectedReport);
    if (rejectedReport.rejected != 1 || rejectedReport.lastRejection != sub0::SendResult::Full ||
        display.readingsReceived != 2) // Local delivery continues despite the full link.
        return false;

    // With the queue drained an echo would be accepted and counted, so the check below can detect one.
    link.queueFull = false;
    route.inject(TemperatureReading{22}); // Incoming traffic still reaches the display and is not echoed.
    return display.readingsReceived == 3 && link.readingsAccepted == 1;
}

int main()
{
    return reportWhenTheLinkIsFull() ? 0 : 1;
}
