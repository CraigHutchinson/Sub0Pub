/** Case: static/dynamic bridge with several dynamic subscribers and churn. Two static receivers (controller,
 *  logger); two dynamic probes (offsets 11 and 13) subscribed at setup; a third, transient probe (offset 17)
 *  exists only for publications where (v & 3) == 0 (subscribed before, unsubscribed after). Order:
 *  controller, logger, then dynamic subscribers in subscription order.
 *  Reference: direct calls to the static receivers plus the same minimal hand-written registry as
 *  static_dynamic_bridge/handwritten.cpp (8 slots, add order, order-preserving remove). */
#include "collapse_case.hpp"

namespace {
struct Sample { uint32_t value; };
struct Controller {
    void receive(const Sample& s) noexcept { COLLAPSE_WORK(s.value * 3U); }
};
struct Logger {
    void receive(const Sample& s) noexcept { ++count; COLLAPSE_WORK(s.value ^ count); }
    uint32_t count = 0;
};
struct DynReceiver {
    virtual void receive(const Sample&) noexcept = 0;
protected:
    ~DynReceiver() = default;
};
struct Registry {
    DynReceiver* entries[8] = {};
    uint32_t count = 0;
    void add(DynReceiver* r) noexcept { if (count < 8) entries[count++] = r; }
    void remove(DynReceiver* r) noexcept
    {
        for (uint32_t i = 0; i < count; ++i)
            if (entries[i] == r)
            {
                for (uint32_t j = i + 1; j < count; ++j)
                    entries[j - 1] = entries[j];
                --count;
                return;
            }
    }
    void publish(const Sample& s) const noexcept { for (uint32_t i = 0; i < count; ++i) entries[i]->receive(s); }
};
struct Probe final : DynReceiver {
    explicit Probe(uint32_t o) noexcept : offset(o) {}
    void receive(const Sample& s) noexcept override { COLLAPSE_WORK(s.value + offset); }
    uint32_t offset;
};
collapse::Slot<Controller> controller;
collapse::Slot<Logger> logger;
collapse::Slot<Registry> registry;
collapse::Slot<Probe> probeA;
collapse::Slot<Probe> probeB;
void publish(const Sample& s) noexcept
{
    controller->receive(s);
    logger->receive(s);
    registry->publish(s);
}
}

COLLAPSE_ENTRY void collapse_setup()
{
    controller.emplace(); logger.emplace(); registry.emplace();
    probeA.emplace(11U); probeB.emplace(13U);
    registry->add(&probeA.get()); registry->add(&probeB.get());
}
COLLAPSE_ENTRY void collapse_publish(uint32_t v)
{
    const Sample s{collapse::arg(v)};
    if ((s.value & 3U) == 0U)
    {
        Probe transient(17U);
        registry->add(&transient);
        publish(s);
        registry->remove(&transient);
    }
    else
        publish(s);
}
COLLAPSE_ENTRY void collapse_teardown()
{
    registry->remove(&probeB.get()); registry->remove(&probeA.get());
    probeB.reset(); probeA.reset(); registry.reset(); logger.reset(); controller.reset();
}
