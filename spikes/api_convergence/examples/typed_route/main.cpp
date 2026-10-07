/** A weather station whose delivery is chosen per message type
 *
 * Use when: you want to write publishers and receivers once, start on the runtime broker, and later move the
 * message types you know to be hot (or fixed) onto direct calls without touching that code.
 * Demonstrates: the spike's Subscribe / Publish / publish(), which follow each message type's topology, and the
 * StaticTo / StaticFirst configuration options that select it, written next to the type they configure. Every
 * source in this directory is shared by all builds; only mode/<mode>/station_types.hpp differs.
 * Story: a thermometer (its own translation unit) publishes a Reading per measurement and an Alarm when it
 * overheats. A display receives readings; an audit log receives both. The same flow must be observed whether
 * the types are brokered (dynamic), wired to fixed receivers (static), wired for readings only (hot_path), wired
 * to the display with the audit log still joining at run time (bridged), or wired from outside a header that
 * cannot be edited (foreign). Three of them are also built as audit builds, which record the run and print, at
 * exit, each type's publishers and receivers and anything that went unheard (see ../audit_findings).
 * Keep in mind: StaticTo names receivers with static storage duration, in delivery order; a brokered type
 * delivers in the order its receivers were constructed. A translation unit that publishes a statically routed
 * type must see the receivers' definitions, which the types header provides. A receiver left out of a StaticTo
 * list is not called (a debug-build check reports it).
 * Run: each convergence_typed_route_<mode> target returns zero when both receivers observed the whole flow; a
 * convergence_typed_route_<mode>_audit target also prints its audit.
 */
#include "station_types.hpp"

#include "audit.hpp"
#include "display.hpp"
#include "station_check.hpp"
#include "thermometer.hpp"

// Static storage is what StaticTo<&display, &audit> requires; the runtime broker is indifferent to it.
Display display;
Audit audit;

int main()
{
    Thermometer thermometer;
    return runStation(thermometer, display, audit) ? 0 : 1;
}
