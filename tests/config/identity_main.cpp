/** Translation units may give their own (TU-local) message types different SUB0PUB_* policy macros.
 *  `config<Opts...>` must then name a different type in each, or the linker merges one TU's definitions
 *  (e.g. its lock guard) into the other. Built twice, once per link order; a merge shows as a hang or a failure.
 */
#include <cstdio>

bool identityUnlocked() noexcept;
bool identityLocked() noexcept;

int main()
{
    const bool unlocked = identityUnlocked();
    const bool locked = identityLocked();
    std::printf("unlocked=%d locked=%d\n", unlocked, locked);
    return unlocked && locked ? 0 : 1;
}
