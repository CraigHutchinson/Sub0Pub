/** Case cross_file: the route's translation unit, the one place that lists the receivers (see app.cpp). */
#include "../route_out_of_line.hpp"

namespace app {
void SampleRoute::receive(const Sample& s) noexcept
{
    sub0::StaticWiring<&controllerA, &controllerB, &logger>::publish(s);
}
}
