/** Case batch: a publisher emits a burst of 64 samples; the receiver accumulates `value * gain` into its own
 *  total, which the application reads once after the burst. When the delivery collapses, the burst is a
 *  reduction loop that the compiler can vectorise; the loop is marked BATCH_LOOP for tools/vectorize_report.py.
 *  The current runtime broker (Subscribe/Publish). */
#include "collapse_case.hpp"
#include "sub0pub/sub0pub.hpp"

namespace {
constexpr uint32_t cBurst = 64;
struct Sample { uint32_t value; };
uint32_t block[cBurst];
void fillBlock() noexcept { for (uint32_t i = 0; i < cBurst; ++i) block[i] = i * 2654435761U; }
struct Accumulator final : sub0::Subscribe<Sample> {
    explicit Accumulator(uint32_t g) noexcept : gain(g) {}
    void receive(const Sample& s) noexcept override { total += s.value * gain; }
    uint32_t gain;
    uint32_t total = 0;
};
struct Sensor : sub0::Publish<Sample> {
    void sendBurst(uint32_t bias) noexcept
    {
        for (uint32_t i = 0; i < cBurst; ++i) // BATCH_LOOP
            sub0::publish(*this, Sample{block[i] + bias});
    }
};
collapse::Slot<Sensor> sensor;
collapse::Slot<Accumulator> accumulator;
}

COLLAPSE_ENTRY void collapse_setup() { fillBlock(); sensor.emplace(); accumulator.emplace(3U); }
COLLAPSE_ENTRY void collapse_publish(uint32_t v)
{
    sensor->sendBurst(collapse::arg(v));
    COLLAPSE_WORK(accumulator->total);
}
COLLAPSE_ENTRY void collapse_teardown() { accumulator.reset(); sensor.reset(); }
