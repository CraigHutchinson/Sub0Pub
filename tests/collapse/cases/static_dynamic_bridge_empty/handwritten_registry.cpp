/** Case: static/dynamic bridge, empty dynamic side. Equal-work reference: a hand-written program that can accept
 *  dynamic subscribers at run time (the same 8-slot, add/remove registry as static_dynamic_bridge/handwritten)
 *  but has none in this scenario, so its publish still walks an empty registry. The bridge variants select it
 *  with `// COLLAPSE_REFERENCE: handwritten_registry`; `handwritten` (no registry at all) prices the capability. */
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
struct Registry { // equal work to the bridges' dynamic side: 8 slots, add order, order-preserving remove
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

collapse::Slot<Controller> controller;
collapse::Slot<Logger> logger;
collapse::Slot<Registry> registry; // dynamic subscribers can join at run time; none has in this scenario
}

COLLAPSE_ENTRY void collapse_setup() { controller.emplace(); logger.emplace(); registry.emplace(); }
COLLAPSE_ENTRY void collapse_publish(uint32_t v)
{
    const Sample s{collapse::arg(v)};
    controller->receive(s);
    logger->receive(s);
    registry->publish(s);
}
COLLAPSE_ENTRY void collapse_teardown() { registry.reset(); logger.reset(); controller.reset(); }
