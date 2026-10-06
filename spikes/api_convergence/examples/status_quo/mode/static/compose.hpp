#pragma once
/** Station composition for static wiring: the receivers are listed where the application composes itself
 *  (see ../../main.cpp). */
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
