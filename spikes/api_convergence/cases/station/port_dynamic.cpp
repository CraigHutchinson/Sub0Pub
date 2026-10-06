// COLLAPSE_REFERENCE: today_dynamic
/** Case station. Candidate "port member", sinks bound to the runtime broker, plain receivers joined by
 *  Subscription. Judged against the current Subscribe/Publish API: it must not cost more. */
#define PORT_MODE PORT_DYNAMIC
#include "port_app.hpp"
