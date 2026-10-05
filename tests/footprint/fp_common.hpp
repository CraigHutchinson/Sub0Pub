#pragma once
/** Shared message and endpoint types for the footprint scenarios (see measure_footprint.py) */
#include "sub0pub/sub0pub.hpp"

struct MsgA { int value; };
struct MsgB { float value; };

template<typename Data>
struct Sink : sub0::Subscribe<Data> {
    Data last{};
    void receive(const Data& d) noexcept override { last = d; }
};

template<typename Data>
struct Source : sub0::Publish<Data> {
    void send(const Data& d) noexcept { sub0::publish(*this, d); }
};

/// Object sizes exported as symbol sizes so they can be read from any target's object file with nm -S
#define SUB0PUB_FP_SIZEOF(name, ...) extern "C" { char fp_sizeof_##name[sizeof(__VA_ARGS__)]; }
