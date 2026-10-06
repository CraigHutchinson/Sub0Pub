#pragma once
/** Station composition: static (see ../../main.cpp).
 *
 * Demonstrates: the bus is a StaticWiring over receivers with static storage; no Subscription is involved.
 * Keep in mind: the receivers move out of the station object, because a StaticWiring names static storage.
 */
#include "participants.hpp"

inline Display stationDisplay;
inline Audit stationAudit;

struct Station
{
    using Bus = sub0::StaticWiring<&stationDisplay, &stationAudit>;

    Display& display = stationDisplay;
    Audit& audit = stationAudit;
    Thermometer<Bus> thermometer{Bus{}};
};
