#pragma once
/** A station's message types, shared by two translation units: each type says beside its definition which
 *  receivers it is wired to, so every unit that can name the type agrees on its topology.
 *  The receivers are declared here and defined in test_station_receivers.cpp; a unit that publishes also needs
 *  their class definitions, which follow the types.
 */
#include "sub0pub/sub0pub.hpp"

namespace station
{
    class Display;
    class Logger;
    extern Display display;
    extern Logger logger;

    struct Reading
    {
        int celsius;
        using sub0_config = sub0::config<sub0::StaticTo<&display, &logger>>;
    };

    struct Fault
    {
        int code;
    };
    /// The other spelling beside the type: a declaration found by argument-dependent lookup
    sub0::config<sub0::StaticTo<&logger>> sub0_config(Fault*);

    class Display final : public sub0::Subscribe<Reading>
    {
    public:
        void receive(const Reading& reading) noexcept; // test_station_receivers.cpp
        int last = 0;
        int count = 0;
    };

    class Logger final : public sub0::SubscribeAll<Reading, Fault>
    {
    public:
        void receive(const Reading& reading) noexcept; // test_station_receivers.cpp
        void receive(const Fault& fault) noexcept;     // test_station_receivers.cpp
        int readings = 0;
        int faults = 0;
    };

    /// A publisher that is an ordinary class, defined in test_station_receivers.cpp
    class Sensor final : public sub0::Publish<Reading>, public sub0::Publish<Fault>
    {
    public:
        void measure(int celsius) noexcept;
    };
}
