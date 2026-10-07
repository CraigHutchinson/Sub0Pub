/** A weather station's messages, receivers and sensor: written once, delivered either way
 *
 * Use when: you are reading main.cpp; this header is the application code that both of its builds share.
 * Demonstrates: Subscribe, SubscribeAll, Publish and publish() written with no knowledge of how a message is
 * delivered, and the one line beside each message type (sub0::StaticTo) that decides it.
 * Story: a display shows temperature readings; a logger counts readings and faults; a sensor publishes a
 * reading, and a fault when the reading is out of range. Nothing below the two message types changes between
 * the builds.
 * Keep in mind: STATION_WIRED exists only so that one source can be built both ways and compared; an
 * application simply writes the sub0_config line, or leaves it out. A listed receiver has static storage and is
 * declared before the type that names it. receive() carries no `override`: the same declaration is the broker's
 * virtual callback in one build and a plain member function, called directly, in the other.
 * Run: compiled into Sub0Pub_Example_promote_to_static_brokered and Sub0Pub_Example_promote_to_static_wired.
 */
#ifndef SUB0PUB_EXAMPLE_PROMOTE_TO_STATIC_STATION_HPP
#define SUB0PUB_EXAMPLE_PROMOTE_TO_STATIC_STATION_HPP

#include "sub0pub/sub0pub.hpp"

#ifndef STATION_WIRED
#define STATION_WIRED 0 // 0: the runtime broker delivers. 1: each message type names its receivers.
#endif

class Display;
class Logger;
extern Display display; // The station's receivers are fixed objects with static storage (defined in main.cpp).
extern Logger logger;

struct TemperatureReading
{
    int celsius;
#if STATION_WIRED
    using sub0_config = sub0::config<sub0::StaticTo<&display, &logger>>; // The promotion: this line.
#endif
};

struct Fault
{
    int code;
#if STATION_WIRED
    using sub0_config = sub0::config<sub0::StaticTo<&logger>>; // And this one.
#endif
};

class Display final : public sub0::Subscribe<TemperatureReading>
{
public:
    void receive(const TemperatureReading& reading) noexcept
    {
        lastCelsius = reading.celsius;
        ++readingsReceived;
    }

    int lastCelsius = 0;
    unsigned readingsReceived = 0;
};

class Logger final : public sub0::SubscribeAll<TemperatureReading, Fault>
{
public:
    void receive(const TemperatureReading&) noexcept { ++readingsLogged; }
    void receive(const Fault& fault) noexcept
    {
        lastFaultCode = fault.code;
        ++faultsLogged;
    }

    unsigned readingsLogged = 0;
    unsigned faultsLogged = 0;
    int lastFaultCode = 0;
};

class Sensor final : public sub0::Publish<TemperatureReading>, public sub0::Publish<Fault>
{
public:
    void measure(int celsius) noexcept
    {
        sub0::publish(*this, TemperatureReading{celsius});
        if (celsius > cMaximumCelsius)
            sub0::publish(*this, Fault{cOverTemperature});
    }

    static constexpr int cMaximumCelsius = 60;
    static constexpr int cOverTemperature = 1;
};

#endif // SUB0PUB_EXAMPLE_PROMOTE_TO_STATIC_STATION_HPP
