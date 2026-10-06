/** Case station. Candidate "typed route", StaticTo, zero-edit promotion: receivers keep `override`, so the
 *  subscriber base keeps a virtual receive(). Prices the vptr and vtable that a zero-edit promotion retains. */
#define SUB0PUB_SPIKE_STATIC_VIRTUAL true
#define ROUTE_OVERRIDE override
#define ROUTE_TOPOLOGY ROUTE_STATIC
#include "route_app.hpp"
