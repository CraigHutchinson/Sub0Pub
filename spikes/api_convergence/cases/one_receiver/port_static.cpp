// COLLAPSE_REFERENCE: handwritten_erased
/** Case one_receiver. Candidate "port member", the sink bound to a wire() result.
 *  Judged against a hand-written context pointer plus function pointer: the price of type erasure itself. */
#define PORT_MODE PORT_STATIC
#include "port_app.hpp"
