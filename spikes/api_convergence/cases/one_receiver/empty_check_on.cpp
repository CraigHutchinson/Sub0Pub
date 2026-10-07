// COLLAPSE_REFERENCE: today_dynamic
/** Case one_receiver. The stand-in broker reporting a publication that reaches nobody (the proposed default).
 *  Judged against the library broker; its difference from empty_check_off is what the check costs. */
#define EMPTY_CHECK_OPTION sub0::spike::ReportNoReceivers
#include "empty_check_app.hpp"
