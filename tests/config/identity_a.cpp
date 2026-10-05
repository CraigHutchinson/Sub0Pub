/** config<Opts...> identity across translation units (see identity_main.cpp): this TU uses the Builtin default */
#include "sub0pub/sub0pub.hpp"

namespace {
struct LocalMsg { int value; using sub0_config = sub0::config<sub0::Scoped>; };
struct Pub final : sub0::Publish<LocalMsg>
{
    using Publish::Publish;
    void send(int v) noexcept { sub0::publish(*this, LocalMsg{v}); }
};
struct Sub final : sub0::Subscribe<LocalMsg>
{
    using Subscribe::Subscribe;
    int received = 0;
    void receive(const LocalMsg&) noexcept override { ++received; }
};
} // namespace

bool identityUnlocked() noexcept
{
    static_assert(std::is_same_v<sub0::config_t<LocalMsg>::Lock, sub0::NoLock>, "");
    sub0::Domain<LocalMsg> domain;
    Pub pub(domain);
    Sub sub(domain);
    pub.send(1);
    pub.send(2);
    return sub.received == 2;
}
