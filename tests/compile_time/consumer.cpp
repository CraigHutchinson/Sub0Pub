// Compile-only workload shared byte-for-byte by both revisions. Each translation unit instantiates the same
// public API with distinct message types, as independently compiled consumer components do. No link is timed.
// PROFILE: 0 = wiring, 1 = broker, 2 = umbrella (same wiring work), 3 = aggregate fingerprinting.
#if PROFILE == 0
#include <sub0pub/wiring.hpp>
#elif PROFILE == 1
#include <sub0pub/broker.hpp>
#elif PROFILE == 3
#include <sub0pub/utility/layout.hpp>
#else
#include <sub0pub/sub0pub.hpp>
#endif

#include <cstddef>
#include <cstdint>
#if PROFILE != 3
#include <tuple>
#endif
#include <utility>

namespace {

template<std::size_t Type, std::size_t Unit = TRANSLATION_UNIT>
struct Message { std::uint32_t value; };

#if PROFILE == 1
template<std::size_t Type, std::size_t Receiver>
struct Listener final : sub0::Subscribe<Message<Type>>
{
    std::uint32_t total = 0;
    void receive(const Message<Type>& message) noexcept override { total += message.value ^ Receiver; }
};

template<std::size_t Type>
struct Source final : sub0::Publish<Message<Type>> {};

template<std::size_t Type, std::size_t... Receiver>
std::uint32_t dispatch(std::uint32_t value, std::index_sequence<Receiver...>)
{
    std::tuple<Listener<Type, Receiver>...> listeners;
    Source<Type> source;
    sub0::publish(source, Message<Type>{value});
    return (std::get<Receiver>(listeners).total + ... + 0U);
}

template<std::size_t... Type>
std::uint32_t consume(std::uint32_t value, std::index_sequence<Type...>)
{
    return (dispatch<Type>(value, std::make_index_sequence<RECEIVER_COUNT>{}) + ... + 0U);
}
#elif PROFILE == 3
// Distinct aggregates at the supported 32-member limit expose arity-instantiation cost.
// Array fingerprinting also exercises recursive element fingerprints.
template<std::size_t Type, std::size_t Unit = TRANSLATION_UNIT>
struct LayoutMessage
{
    std::uint32_t f0, f1, f2, f3, f4, f5, f6, f7;
    std::uint32_t f8, f9, f10, f11, f12, f13, f14, f15;
    std::uint32_t f16, f17, f18, f19, f20, f21, f22, f23;
    std::uint32_t f24, f25, f26, f27, f28, f29, f30, f31;
};

template<std::size_t... Type>
std::uint32_t consume(std::uint32_t value, std::index_sequence<Type...>)
{
    return value + (sub0::utility::hashFingerprint(
        sub0::utility::makeFingerprint<LayoutMessage<Type>[2]>()) + ... + 0U);
}
#else
template<std::size_t Receiver>
struct Listener
{
    std::uint32_t total = 0;
    template<std::size_t Type>
    void receive(const Message<Type>& message) noexcept { total += message.value ^ (Type + Receiver); }
};

template<class Wiring, std::size_t... Type>
void dispatch(Wiring& wiring, std::uint32_t value, std::index_sequence<Type...>)
{
    (wiring.publish(Message<Type>{value}), ...);
    const sub0::Sink<Message<0>> output(wiring);
    const auto copied = output;
    copied.publish(Message<0>{value});
}

template<std::size_t... Receiver>
std::uint32_t consume(std::uint32_t value, std::index_sequence<Receiver...>)
{
    std::tuple<Listener<Receiver>...> listeners;
    auto wiring = sub0::wire(std::get<Receiver>(listeners)...);
    dispatch(wiring, value, std::make_index_sequence<MESSAGE_TYPES>{});
    return (std::get<Receiver>(listeners).total + ... + 0U);
}
#endif

} // namespace

#define SUB0PUB_BENCH_JOIN_INNER(a, b) a##b
#define SUB0PUB_BENCH_JOIN(a, b) SUB0PUB_BENCH_JOIN_INNER(a, b)
std::uint32_t SUB0PUB_BENCH_JOIN(compileWorkload, TRANSLATION_UNIT)(std::uint32_t value)
{
#if PROFILE == 1 || PROFILE == 3
    return consume(value, std::make_index_sequence<MESSAGE_TYPES>{});
#else
    return consume(value, std::make_index_sequence<RECEIVER_COUNT>{});
#endif
}
