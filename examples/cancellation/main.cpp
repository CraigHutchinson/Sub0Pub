/** A primary handler claims commands — ordered fallback delivery
 *
 * Use when: an earlier receiver can handle a message and prevent later receivers from handling it.
 * Demonstrates: ThreadLocalContext enabling cancel(), and cancellation scoped to one publication.
 * Story: PrimaryHandler subscribes before FallbackHandler. It claims commands below 100 and
 * cancels further delivery. Command 200 reaches the fallback; commands 42 and 7 do not.
 * Keep in mind: receiver order is part of this pattern. cancel() does not unsubscribe a receiver
 * or cancel the next publication. Unlike filter(), it stops later receivers for this publication.
 * Run: Sub0Pub_Cancellation prints primary handling of 42/7 and fallback handling of 200.
 */
#include "sub0pub/sub0pub.hpp"
#include <cstdio>

// cancel() needs a publish context: opt in per type (or SUB0PUB_CANCEL for every type)
struct Command { int id; using sub0_config = sub0::config<sub0::ThreadLocalContext>; };

class CommandSource : public sub0::Publish<Command> {
public:
    void send(int id) { sub0::publish(this, Command{id}); }
};

// A handler that "claims" commands by cancelling delivery to later subscribers
class PrimaryHandler : public sub0::Subscribe<Command> {
public:
    void receive(const Command& cmd) noexcept {
        if (cmd.id < 100) {
            std::printf("  [Primary] Handled command %d (cancelling further delivery)\n", cmd.id);
            cancel(); // Stop delivery to FallbackHandler
        } else {
            std::printf("  [Primary] Skipping command %d (letting fallback handle)\n", cmd.id);
        }
    }
};

// A fallback handler that only sees commands not claimed by PrimaryHandler
class FallbackHandler : public sub0::Subscribe<Command> {
public:
    void receive(const Command& cmd) noexcept {
        std::printf("  [Fallback] Handling command %d\n", cmd.id);
    }
};

int main()
{
    std::printf("=== Publish Cancellation ===\n\n");

    CommandSource source;
    PrimaryHandler primary;
    FallbackHandler fallback;

    std::printf("Command 42 (< 100, primary claims it):\n");
    source.send(42);

    std::printf("\nCommand 200 (>= 100, falls through to fallback):\n");
    source.send(200);

    std::printf("\nCommand 7 (primary claims again):\n");
    source.send(7);

    return 0;
}
