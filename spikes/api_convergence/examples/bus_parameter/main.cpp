/** A weather station whose publisher is written against a bus type
 *
 * Use when: you are content for publishers to be templates on their output, and want the composition point to
 * decide, explicitly and locally, whether that output is the runtime broker or a fixed wiring.
 * Demonstrates: plain receivers and a Thermometer<Bus> shared by all builds (participants.hpp); the spike's
 * BrokerBus, which presents the runtime broker as a wiring, and Subscription, which joins a plain receiver to
 * it. Only mode/<mode>/compose.hpp differs between builds.
 * Story: a thermometer publishes a Reading per measurement and an Alarm when it overheats; a display receives
 * readings and an audit log receives both. The station is composed three ways: over the broker (dynamic), over
 * a StaticWiring (static), and over a wire() result owned by the station itself (wired).
 * Keep in mind: the bus is part of every publisher's type, and a dynamic receiver needs a Subscription object
 * with the same lifetime rules as the receiver it refers to.
 * Run: each convergence_bus_parameter_<mode> target returns zero when both receivers observed the whole flow.
 */
#include "compose.hpp"

#include "station_check.hpp"

int main()
{
    Station station;
    return runStation(station.thermometer, station.display, station.audit) ? 0 : 1;
}
