// COLLAPSE_REFERENCE: today_dynamic
/** Case one_receiver. Candidate "bus parameter", Bus = BrokerBus, a plain receiver joined by Subscription.
 *  Judged against the current Subscribe/Publish API: it must not cost more. */
#define BUS_MODE BUS_DYNAMIC
#include "bus_app.hpp"
