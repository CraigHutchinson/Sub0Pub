#pragma once
/** The participants of the port-member station (see main.cpp).
 *
 * Demonstrates: receivers as plain classes, and a publisher that is an ordinary class holding a Sink per
 * message type it publishes.
 */
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

class Thermometer final
{
public:
    /** Binds the thermometer's outputs.
     * @param readings  Where each Reading goes; the wiring behind it must outlive the thermometer.
     * @param alarms    Where each Alarm goes; the wiring behind it must outlive the thermometer.
     */
    Thermometer(sub0::Sink<Reading> readings, sub0::Sink<Alarm> alarms) noexcept : readings_(readings), alarms_(alarms) {}

    void measure(int celsius) noexcept
    {
        readings_.publish(Reading{celsius});
        if (celsius >= cOverheatCelsius)
            alarms_.publish(Alarm{celsius});
    }

private:
    sub0::Sink<Reading> readings_;
    sub0::Sink<Alarm> alarms_;
};
