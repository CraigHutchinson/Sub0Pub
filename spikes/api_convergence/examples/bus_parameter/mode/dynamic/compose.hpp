#pragma once
/** Station composition: dynamic (see ../../main.cpp).
 *
 * Demonstrates: the bus is the runtime broker, and each receiver joins it through a Subscription declared
 * after the receiver it refers to.
 */
#include "sub0pub_spike/broker_bus.hpp"
#include "sub0pub_spike/subscription.hpp"

#include "participants.hpp"

struct Station
{
    using Bus = sub0::spike::BrokerBus;

    Display display;
    Audit audit;
    sub0::spike::Subscription<Display, Reading> displaySubscription{display};
    sub0::spike::Subscription<Audit, Reading, Alarm> auditSubscription{audit};
    Thermometer<Bus> thermometer{Bus{}};
};
