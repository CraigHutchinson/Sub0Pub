#pragma once
/** Station composition for the runtime broker: constructing a participant is the composition (see ../../main.cpp). */
#include "participants.hpp"

struct Station
{
    Display display;
    Audit audit;
    Thermometer thermometer;
};
