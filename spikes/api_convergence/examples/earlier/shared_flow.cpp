/** One message flow, three delivery modes
 *
 * Use when: comparing the current runtime broker with fixed receiver wiring.
 * Demonstrates: the same receiving state and flow checks in broker, runtime-address and fixed-address builds.
 * Story: one thermometer publishes two readings to a display and audit log; both must observe both messages.
 * Keep in mind: only the broker mode gives subscribers independent lifetime-driven registration.
 * Run: each api_convergence_* target returns zero when both receivers observe both readings.
 */
#include "sub0pub/broker.hpp"
#include "sub0pub/wiring.hpp"

#define SUB0PUB_SPIKE_API_DYNAMIC 0
#define SUB0PUB_SPIKE_API_STATIC 1
#define SUB0PUB_SPIKE_API_RUNTIME_BOUND 2

#ifndef SUB0PUB_SPIKE_API
#error "Select SUB0PUB_SPIKE_API_DYNAMIC, SUB0PUB_SPIKE_API_STATIC, or SUB0PUB_SPIKE_API_RUNTIME_BOUND"
#endif

struct Reading
{
    int celsius;
};

struct DisplayState
{
    void receive(const Reading& reading) noexcept
    {
        lastCelsius = reading.celsius;
        ++count;
    }

    int lastCelsius = 0;
    unsigned count = 0;
};

struct AuditState
{
    void receive(const Reading& reading) noexcept
    {
        lastCelsius = reading.celsius;
        ++count;
    }

    int lastCelsius = 0;
    unsigned count = 0;
};

#if SUB0PUB_SPIKE_API == SUB0PUB_SPIKE_API_DYNAMIC
struct Display final : DisplayState, sub0::Subscribe<Reading>
{
    void receive(const Reading& reading) noexcept override { DisplayState::receive(reading); }
};

struct Audit final : AuditState, sub0::Subscribe<Reading>
{
    void receive(const Reading& reading) noexcept override { AuditState::receive(reading); }
};

struct Thermometer final : sub0::Publish<Reading>
{
    void measure(int celsius) noexcept { sub0::publish(*this, Reading{celsius}); }
};

struct Scenario
{
    Display display;
    Audit audit;
    Thermometer thermometer;
};
#elif SUB0PUB_SPIKE_API == SUB0PUB_SPIKE_API_RUNTIME_BOUND
using Display = DisplayState;
using Audit = AuditState;

template<class Output>
struct Thermometer final : sub0::Publisher<Thermometer<Output>, Output>
{
    using sub0::Publisher<Thermometer<Output>, Output>::Publisher;
    void measure(int celsius) noexcept { this->publish(Reading{celsius}); }
};

struct Scenario
{
    Display display;
    Audit audit;
    using Wiring = sub0::Wiring<Display, Audit>;
    Wiring wiring;
    Thermometer<Wiring> thermometer;

    Scenario() noexcept : wiring(display, audit), thermometer(wiring) {}
};
#elif SUB0PUB_SPIKE_API == SUB0PUB_SPIKE_API_STATIC
using Display = DisplayState;
using Audit = AuditState;

Display fixedDisplay;
Audit fixedAudit;
using Wiring = sub0::StaticWiring<&fixedDisplay, &fixedAudit>;

template<class Output>
struct Thermometer final : sub0::Publisher<Thermometer<Output>, Output>
{
    using sub0::Publisher<Thermometer<Output>, Output>::Publisher;
    void measure(int celsius) noexcept { this->publish(Reading{celsius}); }
};

struct Scenario
{
    Display& display = fixedDisplay;
    Audit& audit = fixedAudit;
    Thermometer<Wiring> thermometer{Wiring{}};
};
#else
#error "SUB0PUB_SPIKE_API must select Dynamic, Static, or RuntimeBound"
#endif

bool runFlow(Scenario& scenario) noexcept
{
    scenario.thermometer.measure(20);
    scenario.thermometer.measure(21);

    return scenario.display.count == 2U && scenario.audit.count == 2U &&
        scenario.display.lastCelsius == 21 && scenario.audit.lastCelsius == 21;
}

int main()
{
    Scenario scenario;
    return runFlow(scenario) ? 0 : 1;
}
