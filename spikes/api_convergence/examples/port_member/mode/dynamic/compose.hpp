#pragma once
/** Station composition: dynamic (see ../../main.cpp).
 *
 * Demonstrates: the sinks are bound to the runtime broker, and each receiver joins it through a Subscription.
 */
#include "sub0pub_spike/broker_bus.hpp"
#include "sub0pub_spike/subscription.hpp"

#include "participants.hpp"

struct Station
{
    Display display;
    Audit audit;
    sub0::spike::Subscription<Display, Reading> displaySubscription{display};
    sub0::spike::Subscription<Audit, Reading, Alarm> auditSubscription{audit};
    sub0::spike::BrokerBus bus;
    Thermometer thermometer{sub0::Sink<Reading>{bus}, sub0::Sink<Alarm>{bus}};
};
