/** A broker can be explicitly instantiated (e.g. to export one Data type's broker from a shared library) for every
 *  storage and locking combination: members that only apply to other combinations are not instantiated. */
#include <atomic>

#include "doctest.h"
#include "sub0pub/sub0pub.hpp"

namespace {
struct EiSpin
{
    std::atomic_flag flag = ATOMIC_FLAG_INIT;
    void lock() noexcept { while (flag.test_and_set(std::memory_order_acquire)) {} }
    void unlock() noexcept { flag.clear(std::memory_order_release); }
};
struct EiGlobal { int v; };
struct EiScoped { int v; using sub0_config = sub0::config<sub0::Scoped>; };
struct EiLocked { int v; using sub0_config = sub0::config<sub0::LockWith<EiSpin>>; };
struct EiLockedScoped { int v; using sub0_config = sub0::config<sub0::LockWith<EiSpin>, sub0::Scoped>; };
} // namespace

template class sub0::detail::BrokerImpl<EiGlobal, sub0::config_t<EiGlobal>>;
template class sub0::detail::BrokerImpl<EiScoped, sub0::config_t<EiScoped>>;
template class sub0::detail::BrokerImpl<EiLocked, sub0::config_t<EiLocked>>;
template class sub0::detail::BrokerImpl<EiLockedScoped, sub0::config_t<EiLockedScoped>>;

TEST_CASE("Explicitly instantiated brokers keep their table size") {
    CHECK(sub0::detail::Broker<EiGlobal>::cMaxSubscriptions == SUB0PUB_MAX_SUBSCRIPTIONS);
    CHECK(sub0::detail::Broker<EiLockedScoped>::cMaxSubscriptions == SUB0PUB_MAX_SUBSCRIPTIONS);
}
