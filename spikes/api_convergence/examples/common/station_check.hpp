#pragma once
/** The one flow every station example runs, whichever API and delivery mode built it.
 *
 * Demonstrates: that the application-level behaviour is identical across candidates and modes; only the
 * participants' types differ, which is why this is a template.
 * Story: three measurements, the last one overheating. The display must see all three readings; the audit log
 * must see the three readings and the one alarm.
 */

/** Runs the station flow and checks what each receiver observed.
 * @param thermometer  Publishes a Reading per measure(), and an Alarm when it overheats.
 * @param display      Receives every Reading.
 * @param audit        Receives every Reading and every Alarm.
 * @return True if both receivers observed the whole flow.
 */
template<class Thermometer, class Display, class Audit>
[[nodiscard]] bool runStation(Thermometer& thermometer, const Display& display, const Audit& audit) noexcept
{
    thermometer.measure(20);
    thermometer.measure(21);
    thermometer.measure(95);
    return display.readings() == 3U && display.lastCelsius() == 95 && audit.readings() == 3U && audit.alarms() == 1U;
}
