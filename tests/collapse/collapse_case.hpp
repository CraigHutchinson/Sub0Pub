#pragma once
/** Collapse evidence harness (docs/EVIDENCE.md): shared contract for every case variant
 *
 * A case is one application scenario implemented several ways (variants): `handwritten.cpp` is the
 * equal-work reference without Sub0Pub; the other files implement the same behaviour through a Sub0Pub
 * coding pattern. Every variant defines the same three entry points, which tests/collapse/driver.cpp calls:
 *
 *   collapse_setup()          construct the application objects (construction cost is measured)
 *   collapse_publish(v)       the application's publication call site (the publication path under test)
 *   collapse_teardown()       destroy them (destruction cost is measured)
 *
 * Entry points are extern "C" and not inlined into the driver: they are the boundary of the application code
 * being measured. Everything *inside* them may be inlined and optimised freely.
 *
 * Each case is built in two forms (COLLAPSE_OBSERVABLE):
 *   1 observable-work: receivers change observable state; the result (checksum) must match handwritten
 *   0 removable-work:  receivers do no observable work; ideal code removes the machinery around them
 * Argument side effects (collapse::arg) are observable in both forms and must always be preserved.
 */
#include <cstdint>
#include <new>
#include <type_traits>

#if defined(_MSC_VER)
#define COLLAPSE_ENTRY extern "C" __declspec(noinline)
#else
#define COLLAPSE_ENTRY extern "C" __attribute__((noinline, used))
#endif

#ifndef COLLAPSE_OBSERVABLE
#define COLLAPSE_OBSERVABLE 1
#endif

namespace collapse
{
    /// Observable application state (defined in driver.cpp, so it can never be optimised away)
    extern uint32_t g_state;
    /// Count of argument evaluations: side effects of building the published value
    extern uint32_t g_args;
    /// Transport I/O performed (required behaviour: observable in both forms)
    extern uint32_t g_io;

    /// Build a published value with an observable side effect that must survive any optimisation
    inline uint32_t arg(uint32_t v) noexcept
    {
        ++g_args;
        return v;
    }

    /// A transport's required I/O effect (e.g. writing a frame): kept in the removable form too
    inline void io(uint32_t v) noexcept
    {
        g_io = g_io * 33U + v;
    }

    /// Order-sensitive accumulation, so receiver order and instance selection are part of the checksum
    inline void work(uint32_t v) noexcept
    {
        g_state = g_state * 31U + v;
    }

    /** Storage for an application object constructed in setup and destroyed in teardown, with no engaged flag,
     *  so it adds no cost of its own (unlike std::optional) */
    template<class T>
    class Slot
    {
    public:
        template<class... Args>
        T& emplace(Args&&... args) noexcept { return *new (&storage_) T(static_cast<Args&&>(args)...); }
        void reset() noexcept { get().~T(); }
        T& get() noexcept { return *std::launder(reinterpret_cast<T*>(&storage_)); }
        T* operator->() noexcept { return &get(); }
        T& operator*() noexcept { return get(); }
    private:
        alignas(T) unsigned char storage_[sizeof(T)];
    };
}

#if COLLAPSE_OBSERVABLE
#define COLLAPSE_WORK(expr) ::collapse::work(expr)
#else
#define COLLAPSE_WORK(expr) ((void)sizeof(expr)) // names the operands without evaluating them
#endif

COLLAPSE_ENTRY void collapse_setup();
COLLAPSE_ENTRY void collapse_publish(uint32_t value);
COLLAPSE_ENTRY void collapse_teardown();
