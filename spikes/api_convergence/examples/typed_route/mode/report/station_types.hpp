#pragma once
/** The station's message types: brokered, with the wiring recorded (see ../../main.cpp).
 *
 * Use when: the application was written the easy way, over the runtime broker, and you want to know which of its
 * message types could be wired statically, and how.
 * Demonstrates: RecordWiring, a diagnostic broker. At exit the program prints, for each type, the receivers
 * that subscribed, in delivery order, and whether that set ever changed while the type was being published. A
 * type whose set never changed is a candidate for the StaticTo list in the static build.
 * Keep in mind: it reports what one run did, not what every run can do, and it needs RTTI to name receivers.
 */
#include "sub0pub_spike/wiring_report.hpp"

struct Reading
{
    int celsius;
    using sub0_config = sub0::config<sub0::spike::RecordWiring>;
};

struct Alarm
{
    int celsius;
    using sub0_config = sub0::config<sub0::spike::RecordWiring>;
};

inline constexpr int cOverheatCelsius = 90; ///< A Reading at or above this also raises an Alarm
