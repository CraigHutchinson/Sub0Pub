#pragma once
/** Station topology: static (see ../../main.cpp).
 *
 * Demonstrates: both message types wired to fixed receivers. This header is the whole difference from the
 * dynamic build: the receivers are named before they are defined, each type states its receivers in delivery
 * order, and the receiver definitions follow so that every publishing translation unit can call them directly.
 * Keep in mind: every translation unit must see the same topology (the build contract of any per-type
 * configuration), so a project sets it once, from the build system.
 */
#include "sub0pub_spike/topology.hpp"

#include "messages.hpp"

class Display;
class Audit;
extern Display display;
extern Audit audit;

SUB0PUB_CONFIGURE(Reading, sub0::spike::StaticTo<&display, &audit>);
SUB0PUB_CONFIGURE(Alarm, sub0::spike::StaticTo<&audit>);

#include "audit.hpp"
#include "display.hpp"
