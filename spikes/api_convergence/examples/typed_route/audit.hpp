#pragma once
/** The audit log of the typed-route station (see main.cpp).
 *
 * Demonstrates: one receiver of two message types whose topologies may differ. In the hot_path build its
 * Reading side is a direct call and its Alarm side is a runtime subscription.
 */
#include "station_types.hpp"

class Audit final : public sub0::spike::SubscribeAll<Reading, Alarm>
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
