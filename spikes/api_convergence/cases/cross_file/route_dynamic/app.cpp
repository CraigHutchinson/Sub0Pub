// COLLAPSE_REFERENCE: today_dynamic
/** Case cross_file. Candidate "typed route", no topology option: the runtime broker, receivers implemented in another
 *  translation unit. Judged against the current Subscribe/Publish API: it must not cost more. */
#define ROUTE_TOPOLOGY ROUTE_DYNAMIC
#include "../route_app_source.hpp"
