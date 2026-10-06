#pragma once
/** Station composition: wired, with the wiring owned by the station (see ../../main.cpp).
 *
 * Demonstrates: receivers with ordinary lifetimes bound by wire-style runtime addresses, as a member declared
 * after the receivers it refers to.
 * Keep in mind: member order is the lifetime contract here; the wiring and the thermometer must follow the
 * receivers.
 */
#include "participants.hpp"

struct Station
{
    using Bus = sub0::Wiring<Display, Audit>;

    Display display;
    Audit audit;
    Bus bus{display, audit};
    Thermometer<Bus> thermometer{bus};
};
