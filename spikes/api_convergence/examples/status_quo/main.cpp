/** A weather station written twice: the current APIs, as the baseline
 *
 * Use when: judging the candidates. This is what choosing between the runtime broker and static wiring costs
 * today, with no convergence layer at all.
 * Demonstrates: Subscribe / Publish / publish() in the dynamic build, and plain receivers, a templated publisher
 * and StaticWiring in the static build. Both the participants and the composition differ between builds
 * (mode/<mode>/participants.hpp and mode/<mode>/compose.hpp); only this file and the flow are shared.
 * Story: a thermometer publishes a Reading per measurement and an Alarm when it overheats; a display receives
 * readings and an audit log receives both.
 * Keep in mind: this is the control for the comparison, not a recommendation.
 * Run: each convergence_status_quo_<mode> target returns zero when both receivers observed the whole flow.
 */
#include "compose.hpp"

#include "station_check.hpp"

int main()
{
    Station station;
    return runStation(station.thermometer, station.display, station.audit) ? 0 : 1;
}
