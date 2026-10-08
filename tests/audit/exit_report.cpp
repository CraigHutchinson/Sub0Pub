/** The audit's report at exit, and SUB0PUB_AUDIT_EXIT: main() fails the run, and only the action after the report,
 *  seeing exactly the one finding this program makes, turns it into a pass. */
#include <cstdlib>

#define SUB0PUB_AUDIT true // one translation unit: a real build defines it for all of them, from the build system
#define SUB0PUB_AUDIT_EXIT(findings) std::_Exit((findings) == 1U ? 0 : 1)

#include "sub0pub/sub0pub.hpp"

namespace
{
    struct Alarm { int code; };

    struct Sensor final : sub0::Publish<Alarm>
    {
        void raise() noexcept { sub0::publish(*this, Alarm{7}); }
    };
}

int main()
{
    Sensor sensor;
    sensor.raise(); // nobody is subscribed: the audit's one finding
    return 1;       // replaced by SUB0PUB_AUDIT_EXIT once the report has been written
}
