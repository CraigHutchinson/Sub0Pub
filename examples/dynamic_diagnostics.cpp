/** Attach an optional diagnostic probe beside a cooling controller
 *
 * Use when: a fixed control path needs a small runtime list of borrowed diagnostic receivers.
 * Demonstrates: wire() with DynamicPort<T, 1>, tryAdd(), capacity rejection and remove().
 * Story: the controller first runs alone. A probe joins for one reading; a second probe
 * cannot fit. The first is explicitly removed, and the controller continues without it.
 * Keep in mind: DynamicPort needs unique, non-null receiver pointers, removal before
 * destruction, no mutation during delivery and no concurrent access. For callback
 * self-removal, see scoped_diagnostics.cpp.
 * Run: Sub0Pub_Example_dynamic_diagnostics returns zero when the checks pass.
 */
#include "sub0pub/wiring.hpp"

struct TemperatureReading { int celsius; };

struct CoolingController
{
    unsigned readingsReceived = 0;
    bool fanRunning = false;

    void receive(const TemperatureReading& reading) noexcept
    {
        fanRunning = reading.celsius >= 21;
        ++readingsReceived;
    }
};

using DiagnosticPort = sub0::DynamicPort<TemperatureReading, 1>;

struct DiagnosticProbe final : DiagnosticPort::Receiver
{
    unsigned readingsReceived = 0;
    void receive(const TemperatureReading&) noexcept override { ++readingsReceived; }
};

bool attachAndRemoveAProbe()
{
    CoolingController controller;
    DiagnosticPort diagnostics;
    auto wiring = sub0::wire(controller, diagnostics);

    wiring.publish(TemperatureReading{20}); // The controller works even with no diagnostic receiver.
    if (controller.readingsReceived != 1)
        return false;

    DiagnosticProbe probe;
    DiagnosticProbe waitingProbe;
    if (!diagnostics.tryAdd(&probe) || diagnostics.tryAdd(&waitingProbe))
        return false;

    wiring.publish(TemperatureReading{21});
    if (controller.readingsReceived != 2 || probe.readingsReceived != 1)
        return false;

    // DynamicPort borrows unique, non-null receivers. Remove before destruction, outside delivery.
    // Adding, removing and publishing must not run concurrently.
    diagnostics.remove(&probe);
    wiring.publish(TemperatureReading{22});

    return controller.readingsReceived == 3 && controller.fanRunning && probe.readingsReceived == 1;
}

int main()
{
    return attachAndRemoveAProbe() ? 0 : 1;
}
