/** Without SUB0PUB_AUDIT the audit is two constants: nothing is recorded, and code that asks still compiles. */
#include "sub0pub/sub0pub.hpp"

#if SUB0PUB_AUDIT
#error "this translation unit checks the build without the audit"
#endif

static_assert(sub0::auditFindings() == 0U);
static_assert(sub0::auditReport() == 0U);
