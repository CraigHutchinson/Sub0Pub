/** Footprint: 1 type, 1 publisher, 1 subscriber, 1 publish site (Full, Lock=spin (RTOS-style yield hook)) */
#include "fp_axis.hpp"
#include <atomic>
/// Minimal RTOS-style lock: yield() is the RTOS hook the broker calls while quiescing (no std::this_thread needed)
struct SpinLock
{
    std::atomic_flag flag = ATOMIC_FLAG_INIT;
    void lock() noexcept { while (flag.test_and_set(std::memory_order_acquire)) {} }
    void unlock() noexcept { flag.clear(std::memory_order_release); }
    static void yield() noexcept {}
};

struct MsgA { int value; using sub0_config = sub0::with<fp::Full, sub0::LockWith<SpinLock>>; };

struct SinkA final : sub0::Subscribe<MsgA>
{
    SinkA() noexcept { trySubscribe(); } // concurrent configurations activate explicitly after construction
    ~SinkA() { disconnect(); } // before derived state goes: other threads may publish
    MsgA last{};
    void receive(const MsgA& d) noexcept override { last = d; }
};
struct SourceA : sub0::Publish<MsgA> { void send(const MsgA& d) noexcept { sub0::publish(*this, d); } };

SourceA gSourceA;
SinkA gSinkA;

extern "C" void fp_publish_a(int v) noexcept { gSourceA.send(MsgA{v}); }

extern "C" { char fp_sizeof_Subscribe[sizeof(sub0::Subscribe<MsgA>)]; char fp_sizeof_Publish[sizeof(sub0::Publish<MsgA>)]; }
