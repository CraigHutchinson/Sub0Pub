/** A wired station across translation units: this one defines the receivers, their receive() bodies and the
 *  publisher; test_station.cpp drives it. See station_types.hpp.
 */
#include "station_types.hpp"

namespace station
{
    Display display;
    Logger logger;

    void Display::receive(const Reading& reading) noexcept
    {
        last = reading.celsius;
        ++count;
    }

    void Logger::receive(const Reading&) noexcept { ++readings; }
    void Logger::receive(const Fault&) noexcept { ++faults; }

    void Sensor::measure(int celsius) noexcept
    {
        sub0::publish(*this, Reading{celsius});
        if (celsius < -40)
            sub0::publish(*this, Fault{1});
    }
}
