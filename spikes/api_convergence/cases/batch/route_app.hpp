#pragma once
/** Case batch: a publisher emits a burst of 64 samples; the receiver accumulates `value * gain` into its own
 *  total, which the application reads once after the burst. When the delivery collapses, the burst is a
 *  reduction loop that the compiler can vectorise; the loop is marked BATCH_LOOP for tools/vectorize_report.py.
 *  Candidate "typed route": the participants are written once, in the runtime broker's spelling; the including
 *  variant selects the topology (ROUTE_TOPOLOGY). */
#include "collapse_case.hpp"
#include "sub0pub_spike/topology.hpp"

#define ROUTE_DYNAMIC 0
#define ROUTE_STATIC 1
#define ROUTE_BRIDGED 2

namespace {
constexpr uint32_t cBurst = 64;
struct Sample { uint32_t value; };
uint32_t block[cBurst];
void fillBlock() noexcept { for (uint32_t i = 0; i < cBurst; ++i) block[i] = i * 2654435761U; }
// A real application declares `extern Accumulator accumulator;` here (see cases/station/route_app.hpp).
struct AccumulatorSlot;
extern AccumulatorSlot accumulator;
}

#if ROUTE_TOPOLOGY == ROUTE_STATIC
SUB0PUB_CONFIGURE(Sample, sub0::spike::StaticTo<&accumulator>);
#elif ROUTE_TOPOLOGY == ROUTE_BRIDGED
SUB0PUB_CONFIGURE(Sample, sub0::spike::StaticFirst<&accumulator>);
#endif

namespace {
namespace s0 = sub0::spike;
struct Accumulator final : s0::Subscribe<Sample> {
    explicit Accumulator(uint32_t g) noexcept : gain(g) {}
    void receive(const Sample& s) noexcept { total += s.value * gain; }
    uint32_t gain;
    uint32_t total = 0;
};
struct Sensor : s0::Publish<Sample> {
    void sendBurst(uint32_t bias) noexcept
    {
        for (uint32_t i = 0; i < cBurst; ++i) // BATCH_LOOP
            s0::publish(*this, Sample{block[i] + bias});
    }
};
struct AccumulatorSlot : collapse::Slot<Accumulator> {};
collapse::Slot<Sensor> sensor;
AccumulatorSlot accumulator;
}

COLLAPSE_ENTRY void collapse_setup() { fillBlock(); sensor.emplace(); accumulator.emplace(3U); }
COLLAPSE_ENTRY void collapse_publish(uint32_t v)
{
    sensor->sendBurst(collapse::arg(v));
    COLLAPSE_WORK(accumulator->total);
}
COLLAPSE_ENTRY void collapse_teardown() { accumulator.reset(); sensor.reset(); }
