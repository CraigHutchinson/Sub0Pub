#pragma once
/** The station's message types: bridged (see ../../main.cpp).
 *
 * Demonstrates: promoting one known receiver while the type stays open. The display is called directly; the
 * audit log, and anything else that subscribes at run time, still receives Reading through the runtime broker.
 * Keep in mind: bound receivers are called first, then runtime subscribers in registration order.
 */
#include "sub0pub_spike/topology.hpp"

class Display;
extern Display display;

struct Reading
{
    int celsius;
    using sub0_config = sub0::config<sub0::spike::StaticFirst<&display>>;
};

struct Alarm
{
    int celsius;
};

inline constexpr int cOverheatCelsius = 90; ///< A Reading at or above this also raises an Alarm

#include "display.hpp"
