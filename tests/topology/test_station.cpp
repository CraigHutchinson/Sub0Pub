/** A wired station across translation units: the receivers and the publisher's body live in
 *  test_station_receivers.cpp; this unit only names the types and publishes.
 */
#include "doctest.h"
#include "station_types.hpp"

TEST_CASE("topology: a wired type shared by two translation units is delivered by both") {
    station::Sensor sensor;
    sensor.measure(18);   // published by the other unit
    CHECK(station::display.last == 18);
    CHECK(station::logger.readings == 1);
    CHECK(station::logger.faults == 0);

    struct Probe final : sub0::Publish<station::Reading>
    {
        void send(int celsius) noexcept { sub0::publish(*this, station::Reading{celsius}); }
    };
    Probe probe;
    probe.send(19);       // published by this unit
    CHECK(station::display.last == 19);
    CHECK(station::display.count == 2);
    CHECK(station::logger.readings == 2);

    sensor.measure(-50);  // a Reading, then a Fault: configured by the declaration beside the type
    CHECK(station::logger.faults == 1);
    CHECK(sub0::receiverCount<station::Fault>(sensor) == 1U);
}
