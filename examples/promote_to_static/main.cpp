/** A weather station built twice from one source: runtime subscription, then direct calls
 *
 * Use when: you have working publish/subscribe code and want a message type whose receivers are now known to
 * cost nothing at run time, without rewriting its publishers or its receivers.
 * Demonstrates: sub0::StaticTo beside a message type as the only difference between a brokered and a statically
 * wired build of the same application (station.hpp), and sub0::receiverCount() answering in both.
 * Story: the sensor measures 21 and then 75 degrees. The display and the logger receive both readings, and the
 * logger also receives the over-temperature fault. Both builds must produce exactly this result; the wired
 * build additionally proves at compile time that its receivers hold no subscription state.
 * Keep in mind: promote a type when its receivers are a closed set of objects with static storage. The list is
 * then the whole truth: a subscriber it does not name is never called (a debug build reports it), delivery is
 * in the order of the list, and unsubscribe(), cancel(), Domain and Route are not available for that type. Where
 * some receivers still subscribe and unsubscribe at run time, use sub0::StaticFirst instead (dynamic_diagnostics.cpp). This is a teaching
 * example, not a measurement; docs/EVIDENCE.md has the measured comparison with hand-written calls.
 * Run: Sub0Pub_Example_promote_to_static_brokered and Sub0Pub_Example_promote_to_static_wired each print the
 * form they were built in and return zero when the checks pass.
 */
#include "station.hpp"
#include <cstdio>
#include <type_traits>

Display display;
Logger logger;

#if STATION_WIRED
// Promoted: the Subscribe bases are empty, so a receiver is only its own data and has no virtual table,
// and the number of receivers is a constant the build can check.
static_assert(sizeof(Display) == sizeof(int) + sizeof(unsigned));
static_assert(!std::is_polymorphic_v<Logger>);
static_assert(sub0::Publish<TemperatureReading>::receiverCount() == 2U);
#endif

bool runTheStation()
{
    Sensor sensor;

    // The same question in both builds: counted from the subscription table, or a constant of the program.
    if (sub0::receiverCount<TemperatureReading>(sensor) != 2U || sub0::receiverCount<Fault>(sensor) != 1U)
        return false;

    sensor.measure(21);
    sensor.measure(75); // Out of range: a reading, then a fault.

    return display.readingsReceived == 2 && display.lastCelsius == 75
        && logger.readingsLogged == 2 && logger.faultsLogged == 1 && logger.lastFaultCode == Sensor::cOverTemperature;
}

int main()
{
    const bool passed = runTheStation();
    std::printf("%s station: %s\n", STATION_WIRED ? "statically wired" : "brokered", passed ? "ok" : "FAILED");
    return passed ? 0 : 1;
}
