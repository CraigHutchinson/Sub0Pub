#pragma once
/** The participants of the status-quo station, rewritten for static wiring (see ../../main.cpp). */
#include "sub0pub/wiring.hpp"

#include "messages.hpp"

class Display
{
public:
    void receive(const Reading& reading) noexcept
    {
        lastCelsius_ = reading.celsius;
        ++readings_;
    }

    int lastCelsius() const noexcept { return lastCelsius_; }
    unsigned readings() const noexcept { return readings_; }

private:
    int lastCelsius_ = 0;
    unsigned readings_ = 0;
};

class Audit
{
public:
    void receive(const Reading&) noexcept { ++readings_; }
    void receive(const Alarm&) noexcept { ++alarms_; }

    unsigned readings() const noexcept { return readings_; }
    unsigned alarms() const noexcept { return alarms_; }

private:
    unsigned readings_ = 0;
    unsigned alarms_ = 0;
};

template<class Bus>
class Thermometer final : public sub0::Publisher<Thermometer<Bus>, Bus>
{
public:
    using sub0::Publisher<Thermometer<Bus>, Bus>::Publisher;

    void measure(int celsius) noexcept
    {
        this->publish(Reading{celsius});
        if (celsius >= cOverheatCelsius)
            this->publish(Alarm{celsius});
    }
};
