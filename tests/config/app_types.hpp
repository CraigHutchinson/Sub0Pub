#pragma once
/** Message types demonstrating each configuration binding (used by several TUs) */
#include "sub0pub/sub0pub.hpp"

// (1a) member alias: shortest, part of the type's definition
struct Imu
{
    float x, y, z;
    using sub0_config = sub0::config<sub0::Capacity<2>>;
};

// (1a) lean dispatch: no TLS context, no filter() virtual, direct iteration
struct Lean
{
    int value;
    using sub0_config = sub0::config<sub0::Direct, sub0::NoContext, sub0::NoFilter>;
};

// (1a) scoped storage (issue #5): tables live in sub0::Domain<Session> instances
struct Session
{
    int value;
    using sub0_config = sub0::config<sub0::Scoped>;
};

// Scoped + DirectChecked: the re-entrancy check must be per domain
struct SessionChecked
{
    int value;
    using sub0_config = sub0::config<sub0::Scoped, sub0::DirectChecked>;
};

// (1b) ADL declaration next to a type you own but prefer not to modify; works for enums too
namespace gps
{
    struct Fix { double lat, lon; };
    sub0::config<sub0::Capacity<3>, sub0::NoFilter> sub0_config(Fix*);

    enum class Mode : uint8_t { Off, Acquire, Track };
    sub0::config<sub0::Capacity<1>> sub0_config(Mode*);
}

// (1c) traits for a type you cannot modify
SUB0PUB_CONFIGURE(int, sub0::Capacity<5>);

// Tagged fundamental type: configured through its tag
struct CoreTemp { using sub0_config = sub0::config<sub0::Capacity<6>>; };
using CoreTempC = sub0::Tagged<float, CoreTemp>;

// (2) no per-type configuration: the project default from project_config.hpp
struct Plain { int value; };
