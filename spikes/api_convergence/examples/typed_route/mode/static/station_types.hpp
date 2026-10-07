#pragma once
/** The station's message types: static (see ../../main.cpp).
 *
 * Demonstrates: both types wired to fixed receivers, each by a member alias next to its definition. Compared
 * with the dynamic build this header gains the receivers' forward declarations, one alias per type, and the
 * receivers' definitions for the units that publish. Because the configuration is part of the type, every
 * translation unit that can name the type sees the same topology.
 * Keep in mind: a StaticTo list is the delivery order. A message type that names its receivers depends on
 * their declarations (not their definitions); the two includes at the end are what a publisher's unit needs to
 * call them directly.
 */
#include "sub0pub_spike/topology.hpp"

class Display;
class Audit;
extern Display display;
extern Audit audit;

struct Reading
{
    int celsius;
    using sub0_config = sub0::config<sub0::spike::StaticTo<&display, &audit>>;
};

struct Alarm
{
    int celsius;
    using sub0_config = sub0::config<sub0::spike::StaticTo<&audit>>;
};

inline constexpr int cOverheatCelsius = 90; ///< A Reading at or above this also raises an Alarm

#include "audit.hpp"
#include "display.hpp"
