#pragma once
/** The display of the typed-route station (see main.cpp).
 *
 * Demonstrates: a receiver written once. receive() carries no `override`: it overrides the broker's virtual
 * when Reading is brokered, and is an ordinary member, on a class with no vtable, when Reading is StaticTo.
 * Keep in mind: the signature is still checked in both builds. A receive() that does not accept
 * `const Reading&` leaves the class abstract when brokered and is a compile error when wired.
 */
#include "station_types.hpp"

class Display final : public sub0::spike::Subscribe<Reading>
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
