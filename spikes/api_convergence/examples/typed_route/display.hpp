#pragma once
/** The display of the typed-route station (see main.cpp).
 *
 * Demonstrates: a receiver written once. receive() carries no `override`: it overrides the broker's virtual
 * when Reading is brokered, and is an ordinary member, on a class with no vtable, when Reading is StaticTo.
 */
#include "topology.hpp"

#include "messages.hpp"

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
