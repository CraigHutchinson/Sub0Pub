/** Case one_receiver. Candidate "typed route", StaticFirst: the receiver is promoted to a direct call and the
 *  runtime broker stays reachable for subscribers that join later (none does). Prices the open dynamic side. */
#define ROUTE_TOPOLOGY ROUTE_BRIDGED
#include "route_app.hpp"
