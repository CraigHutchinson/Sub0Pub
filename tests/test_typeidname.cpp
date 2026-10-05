/** SUB0PUB_TYPEIDNAME: user-assigned type identity for inter-process streams (issue #11: did not compile)
 * Types are unique to this translation unit, so the macro affects only them.
 */
#define SUB0PUB_TYPEIDNAME true

#include <string>
#include <thread>

#include "doctest.h"
#include "sub0pub/sub0pub.hpp"

namespace {

struct NamedMsg { int value; };

struct NamedSubscriber : sub0::Subscribe<NamedMsg>
{
    NamedSubscriber() noexcept : sub0::Subscribe<NamedMsg>(0x4E414D45U, "NamedMsg") {}
    int received = 0;
    void receive(const NamedMsg&) noexcept override { ++received; }
};

struct NamedPublisher : sub0::Publish<NamedMsg>
{
    NamedPublisher() noexcept : sub0::Publish<NamedMsg>(0x4E414D45U, "NamedMsg") {}
    void send(int value) noexcept { sub0::publish(*this, NamedMsg{value}); }
};

} // namespace

TEST_CASE("TYPEIDNAME: the assigned identity names the type and its stream header") {
    NamedSubscriber sub;
    NamedPublisher pub;
    CHECK(std::string(sub.typeName()) == "NamedMsg");
    CHECK(pub.typeId() == 0x4E414D45U);
    CHECK(sub0::DefaultSerialisation::Header(NamedMsg{1}).typeId == 0x4E414D45U);
    pub.send(1);
    CHECK(sub.received == 1);
}

namespace {
struct RacedMsg { int value; };
struct RacedPublisher final : sub0::Publish<RacedMsg>
{
    RacedPublisher() noexcept : sub0::Publish<RacedMsg>(0x52414345U, "RacedMsg") {}
};
} // namespace

TEST_CASE("TYPEIDNAME: publishers of one type constructed on several threads at once (run under TSan)") {
    auto construct = [] {
        for (int i = 0; i < 1000; ++i)
        {
            RacedPublisher p;
            (void)p;
        }
    };
    std::thread a(construct), b(construct);
    a.join();
    b.join();
    RacedPublisher p;
    CHECK(p.typeId() == 0x52414345U);
    CHECK(std::string(p.typeName()) == "RacedMsg");
}
