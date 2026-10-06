// COLLAPSE_REFERENCE: handwritten_runtime
/** Case batch: a publisher emits a burst of 64 samples; the receiver accumulates `value * gain` into its own
 *  total, which the application reads once after the burst. When the delivery collapses, the burst is a
 *  reduction loop that the compiler can vectorise; the loop is marked BATCH_LOOP for tools/vectorize_report.py.
 *  The current runtime-bound API: wire(...) held by value in the publisher. */
#include "collapse_case.hpp"
#include "sub0pub/sub0pub.hpp"

namespace {
constexpr uint32_t cBurst = 64;
struct Sample { uint32_t value; };
uint32_t block[cBurst];
void fillBlock() noexcept { for (uint32_t i = 0; i < cBurst; ++i) block[i] = i * 2654435761U; }
struct Accumulator {
    explicit Accumulator(uint32_t g) noexcept : gain(g) {}
    void receive(const Sample& s) noexcept { total += s.value * gain; }
    uint32_t gain;
    uint32_t total = 0;
};
collapse::Slot<Accumulator> accumulator;
using Bus = sub0::Wiring<Accumulator>;
template<class Out>
struct Sensor {
    explicit Sensor(const Out& o) noexcept : out(o) {}
    void sendBurst(uint32_t bias) noexcept
    {
        for (uint32_t i = 0; i < cBurst; ++i) // BATCH_LOOP
            out.publish(Sample{block[i] + bias});
    }
    Out out;
};
collapse::Slot<Sensor<Bus>> sensor;
}

COLLAPSE_ENTRY void collapse_setup() { fillBlock(); accumulator.emplace(3U); sensor.emplace(Bus(accumulator.get())); }
COLLAPSE_ENTRY void collapse_publish(uint32_t v)
{
    sensor->sendBurst(collapse::arg(v));
    COLLAPSE_WORK(accumulator->total);
}
COLLAPSE_ENTRY void collapse_teardown() { sensor.reset(); accumulator.reset(); }
