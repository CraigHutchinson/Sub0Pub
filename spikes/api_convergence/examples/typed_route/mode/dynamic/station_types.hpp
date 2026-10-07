#pragma once
/** The station's message types: dynamic (see ../../main.cpp).
 *
 * Demonstrates: the starting point. Neither type names a topology, so both are delivered by the runtime broker
 * and receivers come and go with their lifetimes.
 */
#include "sub0pub_spike/topology.hpp"

struct Reading
{
    int celsius;
};

struct Alarm
{
    int celsius;
};

inline constexpr int cOverheatCelsius = 90; ///< A Reading at or above this also raises an Alarm
