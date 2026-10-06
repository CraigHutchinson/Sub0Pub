#pragma once
/** Case cross_file, candidate "typed route" with an out-of-line route.
 *  The message type is routed to one object, SampleRoute, whose receive() is declared here and defined in route.cpp,
 *  the only translation unit that names the real receivers. A publisher then needs nothing but this declaration: it
 *  does not see the receivers' definitions, at the price of one call that only link-time optimisation removes.
 *  (A project would split this header: the first half for publishers, the second for the units that own receivers.) */
#include "collapse_case.hpp"
#include "sub0pub_spike/topology.hpp"

namespace app {
struct Sample { uint32_t value; };

/** The route for Sample: all a publisher's translation unit has to see. */
struct SampleRoute {
    void receive(const Sample& s) noexcept;   // defined in route.cpp
};
inline SampleRoute sampleRoute;
} // namespace app

SUB0PUB_CONFIGURE(app::Sample, sub0::spike::StaticTo<&app::sampleRoute>);

// ---- what only the receivers' and the route's translation units need --------------------------------------------
namespace app {
struct Controller final : sub0::spike::Subscribe<Sample> {
    explicit Controller(uint32_t g) noexcept : gain(g) {}
    void receive(const Sample& s) noexcept;   // defined in receivers.cpp
    uint32_t gain;
};

struct Logger final : sub0::spike::Subscribe<Sample> {
    void receive(const Sample& s) noexcept;   // defined in receivers.cpp
    uint32_t count = 0;
};

struct ControllerSlot : collapse::Slot<Controller> {};
struct LoggerSlot : collapse::Slot<Logger> {};
extern ControllerSlot controllerA;
extern ControllerSlot controllerB;
extern LoggerSlot logger;
} // namespace app
