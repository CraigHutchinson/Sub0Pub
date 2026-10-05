/** Sub0Pub: Publisher-side helpers: Sink<T> (type-erased port) and the Publisher<Derived, Out> mixin
 * @remark Part of Sub0Pub (https://github.com/BareCpper/Sub0Pub), MIT License: see LICENSE.md.
 *         Included by the umbrella header <sub0pub/sub0pub.hpp>.
 */
#ifndef CROG_SUB0PUB_WIRING_PUBLISHER_HPP
#define CROG_SUB0PUB_WIRING_PUBLISHER_HPP

#include "sub0pub/wiring/capability.hpp"
#include <concepts>
#include <type_traits>

namespace sub0
{
    /** A type-erased publication port for one message type, for publishers that are not templates (a library or
     *  translation-unit boundary). One indirect call reaches the typed wiring; everything behind it stays static.
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

    /** CRTP publisher mixin for dynamic topology: holds the wiring by value and gives the derived publisher
     *  `this->publish(msg)`: `struct Sensor : sub0::Publisher<Sensor, Bus> { using Publisher::Publisher; ... };`
     */
    template<class Derived, class Out>
    class Publisher
    {
    public:
        constexpr explicit Publisher(const Out& out) noexcept : out_(out) {}

    protected:
        template<class T>
        void publish(const T& msg) const noexcept { out_.publish(msg); }

    private:
        Out out_; // by value: a Wiring is a tuple of receiver references (one hop), a StaticWiring is empty
    };
} // END: sub0

#endif // CROG_SUB0PUB_WIRING_PUBLISHER_HPP
