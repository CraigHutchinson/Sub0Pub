#pragma once
/** The station's message types: wired from outside a header that cannot be edited (see ../../main.cpp).
 *
 * Use when: the message types belong to someone else (a vendor header, a generated file, a fundamental type), so
 * the configuration cannot be written next to them.
 * Demonstrates: SUB0PUB_CONFIGURE, the one spelling that configures a type from outside its definition. Here
 * messages.hpp stands for the header that cannot be edited.
 * Keep in mind: this is the form with a known limitation. The configuration is no longer part of the type, so a
 * translation unit that includes messages.hpp directly, and not this header, sees the same types as brokered
 * and publishes to a broker nobody subscribed to. Prefer the member alias or the declaration beside the type
 * whenever the type is yours.
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
