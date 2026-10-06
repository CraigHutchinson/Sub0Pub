#pragma once
/** Station composition: static (see ../../main.cpp).
 *
 * Demonstrates: the sinks are bound to a wiring over the station's own receivers; no Subscription is involved.
 */
#include "participants.hpp"

struct Station
{
    Display display;
    Audit audit;
    sub0::Wiring<Display, Audit> bus{display, audit};
    Thermometer thermometer{sub0::Sink<Reading>{bus}, sub0::Sink<Alarm>{bus}};
};
