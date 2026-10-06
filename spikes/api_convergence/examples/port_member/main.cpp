/** A weather station whose publisher holds type-erased ports
 *
 * Use when: publishers must stay ordinary, non-template classes that can live behind a library boundary, and
 * one indirect call per publication is an acceptable price in every mode.
 * Demonstrates: plain receivers and a Thermometer holding one Sink per message type, shared by all builds
 * (participants.hpp). The composition point binds the sinks to the runtime broker or to a wire() result; only
 * mode/<mode>/compose.hpp differs between builds.
 * Story: a thermometer publishes a Reading per measurement and an Alarm when it overheats; a display receives
 * readings and an audit log receives both.
 * Keep in mind: the static build is not collapsible. A Sink is an indirect call the compiler cannot see through,
 * so this candidate trades the static path's performance for uniformity.
 * Run: each convergence_port_member_<mode> target returns zero when both receivers observed the whole flow.
 */
#include "compose.hpp"

#include "station_check.hpp"

int main()
{
    Station station;
    return runStation(station.thermometer, station.display, station.audit) ? 0 : 1;
}
