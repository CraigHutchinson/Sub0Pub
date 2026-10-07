#pragma once
/** The station's message types: hot path only (see ../../main.cpp).
 *
 * Demonstrates: optimising one known type. Reading, published on every measurement, is wired to its two
 * receivers; Alarm, which is rare, stays on the runtime broker. The topology is given here by the other
 * next-to-the-type spelling, a declaration found by argument-dependent lookup, which leaves the struct itself
 * untouched.
 * Keep in mind: that declaration must be in the type's own namespace and precede the type's first use; placed
 * anywhere else it is ignored without a diagnostic. Beside the type, as here, it cannot go wrong.
 */
#include "sub0pub_spike/topology.hpp"

class Display;
class Audit;
extern Display display;
extern Audit audit;

struct Reading
{
    int celsius;
};
sub0::config<sub0::spike::StaticTo<&display, &audit>> sub0_config(Reading*);

struct Alarm
{
    int celsius;
};

inline constexpr int cOverheatCelsius = 90; ///< A Reading at or above this also raises an Alarm

#include "audit.hpp"
#include "display.hpp"
