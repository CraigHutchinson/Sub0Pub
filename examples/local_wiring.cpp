/** A thermometer and fault log — wiring known local receivers
 *
 * Use when: receivers are known at composition time but live as ordinary local objects, so a message type
 * cannot name them (sub0::StaticTo needs static storage; see static_addresses.cpp).
 * Demonstrates: wire(), a publisher that holds the wiring it publishes to, handles_v and type-matched
 * receive() overloads.
 * Story: a thermometer sends a reading to a display. The same wiring sends a sensor fault
 * to the fault log; receivers without a matching receive() are skipped.
 * Keep in mind: wiring borrows its receivers, which must outlive it and the publisher.
 * No runtime subscription table is involved.
 * Run: Sub0Pub_Example_local_wiring returns zero when the checks pass.
 */
#include "sub0pub/wiring.hpp"

struct TemperatureReading { int celsius; };

struct SensorFault { unsigned code; };

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

struct FaultLog
{
    unsigned faultsReceived = 0;
    void receive(const SensorFault&) noexcept { ++faultsReceived; }
};

template<class Output>
struct TemperatureSensor final
{
    Output output; // the wiring, held by value: one reference per receiver
    void measure(int celsius) noexcept { output.publish(TemperatureReading{celsius}); }
};

bool wireLocalReceivers()
{
    TemperatureDisplay display;
    FaultLog faultLog;
    auto wiring = sub0::wire(display, faultLog);
    TemperatureSensor<decltype(wiring)> thermometer{wiring};
    static_assert(sub0::handles_v<TemperatureDisplay, TemperatureReading>, "TemperatureDisplay must handle TemperatureReading");

    thermometer.measure(20);
    wiring.publish(SensorFault{1}); // Each message reaches only receivers with a matching receive().

    return display.lastCelsius == 20 && display.readingsReceived == 1 && faultLog.faultsReceived == 1;
}

int main()
{
    return wireLocalReceivers() ? 0 : 1;
}
