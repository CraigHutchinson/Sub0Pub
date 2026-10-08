/** Sub0Pub: Sink<T>: a type-erased publication port into a wiring
 * @remark Part of Sub0Pub (https://github.com/BareCpper/Sub0Pub), MIT License: see LICENSE.md.
 *         Included by the umbrella header <sub0pub/sub0pub.hpp>.
 */
#ifndef CROG_SUB0PUB_WIRING_SINK_HPP
#define CROG_SUB0PUB_WIRING_SINK_HPP

#include "sub0pub/wiring/capability.hpp"
#include <concepts>
#include <type_traits>

namespace sub0
{
    /** A type-erased publication port for one message type, for a publisher that must not name its wiring: one
     *  compiled into a library, or behind an interface that cannot be a template. One indirect call reaches the
     *  typed wiring; everything behind it stays static.
     * @remark A publisher in the application's own source does not need one: it derives from Publish<T> and its
     *         message type's configuration decides the delivery (StaticTo), or it holds the wiring it publishes to.
     */
    template<class T>
    class Sink
    {
    public:
        /// Wrap a wiring, which must outlive the Sink (constrained: copying a Sink copies it, never wraps it)
        template<class W>
            requires (!std::same_as<std::remove_cv_t<W>, Sink>)
        explicit Sink(W& wiring) noexcept
            : target_(&wiring)
            , call_([](const void* w, const T& msg) noexcept { static_cast<const W*>(w)->publish(msg); })
        {}

        void publish(const T& msg) const noexcept { call_(target_, msg); }

    private:
        const void* target_;
        void (*call_)(const void*, const T&) noexcept;
    };
} // END: sub0

#endif // CROG_SUB0PUB_WIRING_SINK_HPP
