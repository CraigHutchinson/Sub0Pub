#pragma once
/** The participants of the status-quo station, written for the runtime broker (see ../../main.cpp). */
#include "sub0pub/broker.hpp"

#include "messages.hpp"

class Display final : public sub0::Subscribe<Reading>
{
public:
    void receive(const Reading& reading) noexcept override
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

class Audit final : public sub0::SubscribeAll<Reading, Alarm>
{
public:
    void receive(const Reading&) noexcept override { ++readings_; }
    void receive(const Alarm&) noexcept override { ++alarms_; }

    unsigned readings() const noexcept { return readings_; }
    unsigned alarms() const noexcept { return alarms_; }

private:
    unsigned readings_ = 0;
    unsigned alarms_ = 0;
};

class Thermometer final : public sub0::Publish<Reading>, public sub0::Publish<Alarm>
{
public:
    void measure(int celsius) noexcept
    {
        sub0::publish(*this, Reading{celsius});
        if (celsius >= cOverheatCelsius)
            sub0::publish(*this, Alarm{celsius});
    }
};
