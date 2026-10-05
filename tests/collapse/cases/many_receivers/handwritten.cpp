/** Case: 32 receivers of one type (fan-out scaling). Reference: one direct call per receiver, in order
 *  c0..c31 (gain = index + 1). */
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
}

#define MANY_EMPLACE(i) c##i.emplace(i + 1U);
#define MANY_RESET(i) c##i.reset();
#define MANY_CALL(i) c##i->receive(s);
COLLAPSE_ENTRY void collapse_setup() { MANY_EACH(MANY_EMPLACE) }
COLLAPSE_ENTRY void collapse_publish(uint32_t v)
{
    const Sample s{collapse::arg(v)};
    MANY_EACH(MANY_CALL)
}
COLLAPSE_ENTRY void collapse_teardown() { MANY_EACH(MANY_RESET) }
