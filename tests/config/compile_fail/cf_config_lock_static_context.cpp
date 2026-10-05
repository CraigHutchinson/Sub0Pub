// EXPECT: "a Lock requires ThreadLocalContext"
// StaticContext keeps one frame stack for the process: with concurrent publishers (a Lock), one thread's cancel()
// and dispatch frames act on another thread's publication (reproduced under TSan: data races and lost deliveries).
#include "sub0pub/sub0pub.hpp"
struct SpinLock { void lock() noexcept {} void unlock() noexcept {} };
struct Msg { using sub0_config = sub0::config<sub0::LockWith<SpinLock>, sub0::StaticContext>; };
struct S : sub0::Subscribe<Msg> { void receive(const Msg&) noexcept override {} };
int main() { S s; }
