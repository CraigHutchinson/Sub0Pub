/** The thermometer's translation unit (see main.cpp for the station's story).
 *
 * Demonstrates: publishing from a separate translation unit. For a StaticTo type the calls below compile to
 * direct calls into the receivers, whose definitions this file sees through the topology header; for a brokered
 * type they are the runtime broker's publish.
 */
#include "thermometer.hpp"

void Thermometer::measure(int celsius) noexcept
{
    sub0::spike::publish(*this, Reading{celsius});
    if (celsius >= cOverheatCelsius)
        sub0::spike::publish(*this, Alarm{celsius});
}
