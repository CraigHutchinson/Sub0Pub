#pragma once
/** Station topology: bridged (see ../../main.cpp).
 *
 * Demonstrates: promoting one known receiver while the type stays open. The display is called directly; the
 * audit log, and anything else that subscribes at run time, still receives Reading through the runtime broker.
 * Keep in mind: bound receivers are called first, then runtime subscribers in registration order.
 */
#include "sub0pub_spike/topology.hpp"

#include "messages.hpp"

class Display;
extern Display display;

SUB0PUB_CONFIGURE(Reading, sub0::spike::StaticFirst<&display>);

#include "display.hpp"
