// COLLAPSE_REFERENCE: today_dynamic
/** Case one_receiver. Candidate "port member", the sink bound to the runtime broker, a plain receiver joined by
 *  Subscription. Judged against the current Subscribe/Publish API: it must not cost more. */
#define PORT_MODE PORT_DYNAMIC
#include "port_app.hpp"
