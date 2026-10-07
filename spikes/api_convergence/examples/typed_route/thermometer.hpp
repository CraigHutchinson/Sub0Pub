#pragma once
/** The thermometer of the typed-route station (see main.cpp).
 *
 * Demonstrates: a publisher that is an ordinary class, declared here and defined in thermometer.cpp, with no
 * template parameter for its output and no knowledge of who receives.
 */
#include "station_types.hpp"

class Thermometer final : public sub0::spike::Publish<Reading>, public sub0::spike::Publish<Alarm>
{
public:
    /** Publishes a Reading, and an Alarm when the reading is at or above cOverheatCelsius.
     * @param celsius  The measured temperature.
     */
    void measure(int celsius) noexcept;
};
