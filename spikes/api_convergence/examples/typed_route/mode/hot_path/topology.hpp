#pragma once
/** Station topology: hot path only (see ../../main.cpp).
 *
 * Demonstrates: optimising one known type. Reading, published on every measurement, is wired to its two
 * receivers; Alarm, which is rare, stays on the runtime broker. One configuration line is the whole change.
 */
#include "sub0pub_spike/topology.hpp"

#include "messages.hpp"

class Display;
class Audit;
extern Display display;
extern Audit audit;

SUB0PUB_CONFIGURE(Reading, sub0::spike::StaticTo<&display, &audit>);

#include "audit.hpp"
#include "display.hpp"
