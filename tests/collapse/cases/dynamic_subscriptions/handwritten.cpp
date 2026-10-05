/** Case: dynamic subscriptions (the genuinely dynamic boundary). A controller is always subscribed; on every
 *  fourth publication a transient probe subscribes, receives, and unsubscribes. Reference: a minimal hand-written
 *  runtime registry (fixed table, virtual receive, ordered removal), i.e. the least a dynamic design must pay.
 *  This case sets regression limits for runtime subscription; it is not expected to match static dispatch. */
#include "collapse_case.hpp"

namespace {
struct Sample { uint32_t value; };
struct Receiver {
    virtual void receive(const Sample& s) noexcept = 0;
protected:
    ~Receiver() = default;
};
struct Registry {
    Receiver* entries[8] = {};
    uint32_t count = 0;
    void add(Receiver* r) noexcept { if (count < 8) entries[count++] = r; }
    void remove(Receiver* r) noexcept
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
    void publish(const Sample& s) noexcept { for (uint32_t i = 0; i < count; ++i) entries[i]->receive(s); }
};
struct Controller final : Receiver {
    void receive(const Sample& s) noexcept override { COLLAPSE_WORK(s.value * 3U); }
};
struct Probe final : Receiver {
    void receive(const Sample& s) noexcept override { COLLAPSE_WORK(s.value + 11U); }
};
collapse::Slot<Registry> registry;
collapse::Slot<Controller> controller;
}

COLLAPSE_ENTRY void collapse_setup() { registry.emplace(); controller.emplace(); registry->add(&controller.get()); }
COLLAPSE_ENTRY void collapse_publish(uint32_t v)
{
    const Sample s{collapse::arg(v)};
    if ((s.value & 3U) == 0U)
    {
        Probe probe;
        registry->add(&probe);
        registry->publish(s);
        registry->remove(&probe);
    }
    else
        registry->publish(s);
}
COLLAPSE_ENTRY void collapse_teardown() { registry->remove(&controller.get()); controller.reset(); registry.reset(); }
