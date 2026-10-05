/** Static wiring: guarantees and limitations that the collapse cases cannot show, because a case must behave
 *  exactly like its hand-written reference (docs/EVIDENCE.md). Tests named "limitation" pin a documented
 *  limitation (K14, K18-K24 in docs/DESIGN.md, "Known limitations"), so a change of behaviour is noticed. */
#include "doctest.h"
#include "sub0pub/sub0pub.hpp"

#include <cstdint>
#include <vector>

namespace {

struct Sample { uint32_t value; };
struct Command { uint32_t code; };
struct Other { uint32_t value; };

std::vector<int> gTrace; // order-sensitive delivery log shared by the receivers below

struct Controller
{
    int id;
    void receive(const Sample& s) noexcept { gTrace.push_back(id * 1000 + static_cast<int>(s.value)); }
};

struct Radio
{
    int sent = 0;
    void send(const Sample&) noexcept { ++sent; }
};

// ---------------------------------------------------------------------------------------------------------
// Split horizon (transport endpoints)
// ---------------------------------------------------------------------------------------------------------
Controller sController{1};
Radio sRadio;
sub0::StaticForward<&sRadio> sUplink;
using SBus = sub0::StaticWiring<&sController, &sUplink>;

TEST_CASE("split horizon: ingress may name the transport or its adapter as origin")
{
    // Regression: naming the transport object (the natural origin at a receive callback) was once not
    // recognised as the Forward/StaticForward bound for it, and ingress was echoed back out, silently.
    gTrace.clear();
    sRadio.sent = 0;
    SBus::publishFrom(sRadio, Sample{7});
    CHECK(sRadio.sent == 0);
    SBus::publishFrom(sUplink, Sample{8});
    CHECK(sRadio.sent == 0);
    SBus::publishFrom<Radio>(Sample{9}); // by type: the transport type is bound once, through its adapter
    CHECK(sRadio.sent == 0);
    CHECK(gTrace == std::vector<int>{1007, 1008, 1009});
    SBus::publish(Sample{1}); // egress still reaches the transport
    CHECK(sRadio.sent == 1);

    Controller c{2};
    Radio r;
    sub0::Forward<Radio> link(r);
    const auto bus = sub0::wire(c, link);
    bus.publishFrom(r, Sample{1});
    bus.publishFrom(link, Sample{2});
    bus.publishFrom<Radio>(Sample{3});
    bus.publishFrom<sub0::Forward<Radio>>(Sample{4});
    CHECK(r.sent == 0);
    bus.publish(Sample{5});
    CHECK(r.sent == 1);
}

TEST_CASE("split horizon: two links of one transport type are told apart at run time")
{
    Controller c{3};
    Radio a;
    Radio b;
    sub0::Forward<Radio> linkA(a);
    sub0::Forward<Radio> linkB(b);
    const auto bus = sub0::wire(c, linkA, linkB);
    bus.publishFrom(a, Sample{1}); // only link A is skipped: an address compare per same-typed endpoint
    CHECK(a.sent == 0);
    CHECK(b.sent == 1);
    bus.publishFrom(linkB, Sample{2});
    CHECK(a.sent == 1);
    CHECK(b.sent == 1);
}

// ---------------------------------------------------------------------------------------------------------
// K14: capability routing is silent on a signature mismatch
// ---------------------------------------------------------------------------------------------------------
struct WrongParam
{
    int got = 0;
    void receive(const Other&) noexcept { ++got; } // meant to receive Sample
};
struct NonConstReceive
{
    int got = 0;
    void receive(const Sample&) noexcept { ++got; }
};
const NonConstReceive sConstBound{};
template<class T>
struct Holder // application storage exposing its object through get(), as collapse::Slot does
{
    T object;
    T& get() noexcept { return object; }
};
using ConstBus = sub0::StaticWiring<&sConstBound>;

TEST_CASE("limitation K14: a receiver whose receive() does not match is skipped without a diagnostic")
{
    WrongParam wrong;
    const auto bus = sub0::wire(wrong);
    bus.publish(Sample{1}); // compiles, delivers nothing
    CHECK(wrong.got == 0);
    ConstBus::publish(Sample{1}); // bound through a pointer to const: the non-const receive() is not viable
    CHECK(sConstBound.got == 0);

    // The opt-in guard turns both mistakes into compile-time facts
    static_assert(!sub0::handles_v<WrongParam, Sample>, "detects the wrong parameter type");
    static_assert(!sub0::handles_v<const NonConstReceive, Sample>, "detects the const binding");
    static_assert(sub0::handles_v<NonConstReceive, Sample>, "a matching receiver is accepted");
    static_assert(sub0::handles_v<Holder<NonConstReceive>, Sample>, "holders expose their receiver");
}

NonConstReceive sArray[2];
using ArrayBus = sub0::StaticWiring<&sArray>;

TEST_CASE("limitation K14/K20: binding an array of receivers binds nothing (no loop form)")
{
    ArrayBus::publish(Sample{1}); // the array itself has no receive(): skipped like any non-receiver
    CHECK(sArray[0].got == 0);
    CHECK(sArray[1].got == 0);
    static_assert(!sub0::handles_v<NonConstReceive[2], Sample>, "the guard catches it");
    // A runtime wiring supports one binding per element portably. C++23 array-element NTTP support varies
    // by compiler, so it is no longer a portable compile-fail expectation.
    const auto bus = sub0::wire(sArray[0], sArray[1]);
    bus.publish(Sample{1});
    CHECK(sArray[0].got == 1);
    CHECK(sArray[1].got == 1);
}

// ---------------------------------------------------------------------------------------------------------
// Cancellation (bool-returning receive)
// ---------------------------------------------------------------------------------------------------------
struct Gate
{
    int calls = 0;
    bool receive(const Sample& s) noexcept { ++calls; return (s.value % 3U) != 0U; } // false stops
};
struct FilteredGate
{
    int calls = 0;
    bool filter(const Sample& s) const noexcept { return (s.value & 1U) != 0U; } // odd values only
    bool receive(const Sample&) noexcept { ++calls; return false; }              // always stops when reached
};

Gate sGate;
Controller sAfter{4};
using GateBus = sub0::StaticWiring<&sGate, &sAfter>;

TEST_CASE("cancellation: publishCancelable stops later receivers; plain publish() ignores the result (K19)")
{
    gTrace.clear();
    GateBus::publishCancelable(Sample{3}); // Gate returns false: sAfter not called
    GateBus::publishCancelable(Sample{4});
    CHECK(gTrace == std::vector<int>{4004});
    gTrace.clear();
    GateBus::publish(Sample{3}); // limitation: the same receiver bound to a plain publish() cannot stop it
    CHECK(gTrace == std::vector<int>{4003});

    Gate g;
    Controller after{5};
    const auto bus = sub0::wire(g, after); // a runtime-bound wiring has publishCancelable too
    gTrace.clear();
    bus.publishCancelable(Sample{6});
    bus.publishCancelable(Sample{7});
    CHECK(gTrace == std::vector<int>{5007});
}

TEST_CASE("cancellation combined with a filter: a filtered-out receiver cannot stop the publication")
{
    FilteredGate g;
    Controller after{6};
    const auto bus = sub0::wire(g, after);
    gTrace.clear();
    bus.publishCancelable(Sample{2}); // filtered out: continues
    bus.publishCancelable(Sample{3}); // passes the filter, returns false: stops
    CHECK(g.calls == 1);
    CHECK(gTrace == std::vector<int>{6002});
}

// ---------------------------------------------------------------------------------------------------------
// C++23 constraint migration: preserve explicit-bool filters, exact-bool cancellation and Sink copying
// ---------------------------------------------------------------------------------------------------------
struct ExplicitDecision
{
    bool value;
    explicit operator bool() const noexcept { return value; }
};
struct ProxyReceiver
{
    int calls = 0;
    ExplicitDecision filter(const Sample& sample) const noexcept { return {sample.value != 0}; }
    ExplicitDecision receive(const Sample&) noexcept { ++calls; return {false}; }
};

TEST_CASE("capabilities preserve explicit-bool filters without treating proxy results as cancellation")
{
    ProxyReceiver receiver;
    Controller tail{9};
    auto bus = sub0::wire(receiver, tail);
    static_assert(sub0::handles_v<ProxyReceiver, Sample>);
    static_assert(!sub0::handles_v<ProxyReceiver, Other>);
    gTrace.clear();
    bus.publishCancelable(Sample{0});
    bus.publishCancelable(Sample{1});
    CHECK(receiver.calls == 1);
    CHECK(gTrace == std::vector<int>{9000, 9001});
}

TEST_CASE("copying a Sink keeps the wiring binding after the original Sink is destroyed")
{
    NonConstReceive receiver;
    auto bus = sub0::wire(receiver);
    auto copied = [&bus] {
        const sub0::Sink<Sample> original(bus);
        return sub0::Sink<Sample>(original);
    }();
    copied.publish(Sample{7});
    CHECK(receiver.got == 1);
}

// ---------------------------------------------------------------------------------------------------------
// Nested publication on the static path
// ---------------------------------------------------------------------------------------------------------
struct Relay;
struct Actuator
{
    void receive(const Command& c) noexcept { gTrace.push_back(-static_cast<int>(c.code)); }
};
struct Relay
{
    void receive(const Sample& s) noexcept; // publishes a Command, and once more a Sample, on its own wiring
};
Relay sRelay;
Actuator sActuator;
Controller sTail{7};
using NestBus = sub0::StaticWiring<&sRelay, &sActuator, &sTail>;
void Relay::receive(const Sample& s) noexcept
{
    gTrace.push_back(static_cast<int>(s.value));
    NestBus::publish(Command{s.value + 100U});
    if ((s.value & 1U) != 0U)
        NestBus::publish(Sample{s.value + 1U}); // re-entrant, same type, bounded by the application
}

TEST_CASE("nested publish: a receiver may publish another type, and the same type, on its own static wiring")
{
    gTrace.clear();
    NestBus::publish(Sample{1});
    // 1 -> Command 101 -> re-entrant Sample 2 (-> Command 102, tail 2) -> tail 1
    CHECK(gTrace == std::vector<int>{1, -101, 2, -102, 7002, 7001});
}

// ---------------------------------------------------------------------------------------------------------
// Static/dynamic bridge
// ---------------------------------------------------------------------------------------------------------
struct Probe final : sub0::DynamicPort<Sample, 2>::Receiver
{
    int id;
    sub0::DynamicPort<Sample, 2>* port = nullptr;
    bool leaveOnReceive = false;
    explicit Probe(int i) noexcept : id(i) {}
    void receive(const Sample& s) noexcept override
    {
        gTrace.push_back(id * 1000 + static_cast<int>(s.value));
        if (leaveOnReceive)
            port->remove(this);
    }
};

TEST_CASE("DynamicPort: capacity is reported by tryAdd and silent through add")
{
    sub0::DynamicPort<Sample, 2> port;
    Probe a{1};
    Probe b{2};
    Probe c{3};
    CHECK(port.tryAdd(&a));
    CHECK(port.tryAdd(&b));
    CHECK_FALSE(port.tryAdd(&c));
    port.add(&c); // dropped without a report
    gTrace.clear();
    port.receive(Sample{1});
    CHECK(gTrace == std::vector<int>{1001, 2001});
}

TEST_CASE("limitation K21: a DynamicPort receiver removing itself during delivery makes the next one miss it")
{
    sub0::DynamicPort<Sample, 2> port;
    Probe a{1};
    Probe b{2};
    a.port = &port;
    a.leaveOnReceive = true;
    port.add(&a);
    port.add(&b);
    gTrace.clear();
    port.receive(Sample{1}); // no snapshot: b shifts into a's slot, the loop index moves past it
    CHECK(gTrace == std::vector<int>{1001});
    port.receive(Sample{2});
    CHECK(gTrace == std::vector<int>{1001, 2002});
}

struct BridgeSample
{
    uint32_t value;
    using sub0_config = sub0::config<sub0::Snapshot, sub0::StaticContext>; // policy on the dynamic side only
};
struct BrokerProbe final : sub0::Subscribe<BridgeSample>
{
    int id;
    bool leaveOnReceive = false;
    explicit BrokerProbe(int i) noexcept : id(i) {}
    void receive(const BridgeSample& s) noexcept override
    {
        gTrace.push_back(id * 1000 + static_cast<int>(s.value));
        if (leaveOnReceive)
            this->disconnect();
    }
};

TEST_CASE("BrokerPort with Snapshot dispatch: self-removal during delivery does not skip the next subscriber")
{
    sub0::BrokerPort<BridgeSample> port;
    BrokerProbe a{1};
    BrokerProbe b{2};
    a.leaveOnReceive = true;
    Controller unused{0};
    const auto bus = sub0::wire(unused, port); // Controller does not handle BridgeSample: skipped at compile time
    gTrace.clear();
    bus.publish(BridgeSample{1});
    CHECK(gTrace == std::vector<int>{1001, 2001});
    bus.publish(BridgeSample{2});
    CHECK(gTrace == std::vector<int>{1001, 2001, 2002});
}

} // namespace
