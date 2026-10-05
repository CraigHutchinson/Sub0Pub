/** Case: 32 receivers of one type. Runtime-bound reference: the publisher holds the receivers' addresses,
 *  stored at setup, and calls through each one in order (unrolled, as `wire(...)` delivers). Equal work for B1,
 *  which selects it with `// COLLAPSE_REFERENCE: handwritten_runtime`. */
#include "collapse_case.hpp"
#include "many.hpp"

namespace {
struct Sample { uint32_t value; };
struct Controller {
    explicit Controller(uint32_t g) noexcept : gain(g) {}
    void receive(const Sample& s) noexcept { COLLAPSE_WORK(s.value * gain); }
    uint32_t gain;
};
#define MANY_DECLARE(i) collapse::Slot<Controller> c##i;
MANY_EACH(MANY_DECLARE)
struct Sensor {
    Controller* targets[MANY_COUNT];
    void send(uint32_t v) noexcept
    {
        const Sample s{v};
#define MANY_CALL(i) targets[i]->receive(s);
        MANY_EACH(MANY_CALL)
    }
};
collapse::Slot<Sensor> sensor;
}

#define MANY_EMPLACE(i) c##i.emplace(i + 1U);
#define MANY_RESET(i) c##i.reset();
#define MANY_ADDRESS_OF(i) &c##i.get(),
COLLAPSE_ENTRY void collapse_setup() { MANY_EACH(MANY_EMPLACE) sensor.emplace(Sensor{{MANY_EACH(MANY_ADDRESS_OF)}}); }
COLLAPSE_ENTRY void collapse_publish(uint32_t v) { sensor->send(collapse::arg(v)); }
COLLAPSE_ENTRY void collapse_teardown() { sensor.reset(); MANY_EACH(MANY_RESET) }
