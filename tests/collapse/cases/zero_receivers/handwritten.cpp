/** Case: zero receivers. Reference: nothing receives; only the argument's side effect remains. */
#include "collapse_case.hpp"

COLLAPSE_ENTRY void collapse_setup() {}
COLLAPSE_ENTRY void collapse_publish(uint32_t v) { (void)collapse::arg(v); }
COLLAPSE_ENTRY void collapse_teardown() {}
