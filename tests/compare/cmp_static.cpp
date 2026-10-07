/** v1 vs v2 comparison: v2 static wiring (sub0pub/wiring/) and hand-written code.
 *
 * Same control conditions as cmp_sub0pub.cpp: the publish entry point and every receive() are out of line,
 * so each variant pays one real call per delivery and the difference is the dispatch around it.
 * Static-wiring receivers stop a publication by returning false (publishCancelable); it has no filter() and no
 * per-subscriber lifetime, so those rows are n/a.
 */
#define ANKERL_NANOBENCH_IMPLEMENT
#include "sub0pub/sub0pub.hpp"

#include "cmp_common.hpp"

struct Sample { int v; };

namespace cmp_types {
// Every receiver does the same observable work (one increment) as in cmp_sub0pub.cpp: an empty out-of-line
// receive() would let the optimiser drop the call, which a virtual call never allows
struct NoOp {
    int count = 0;
    CMP_NOINLINE void receive(const Sample&) noexcept;
};
struct Gate { // stops the rest of a cancellable publication
    int count = 0;
    CMP_NOINLINE bool receive(const Sample&) noexcept;
};
struct IReceiver {
    virtual void receive(const Sample&) noexcept = 0;
};
struct VirtualNoOp final : IReceiver {
    int count = 0;
    void receive(const Sample&) noexcept override;
};
void NoOp::receive(const Sample&) noexcept { ++count; }
bool Gate::receive(const Sample&) noexcept { ++count; return false; }
void VirtualNoOp::receive(const Sample&) noexcept { ++count; }

NoOp r0, r1, r2, r3, r4, r5, r6, r7;
Gate gate;

// B2: static addresses, static types
using Static1 = sub0::StaticWiring<&r0>;
using Static8 = sub0::StaticWiring<&r0, &r1, &r2, &r3, &r4, &r5, &r6, &r7>;
using StaticCancel8 = sub0::StaticWiring<&gate, &r1, &r2, &r3, &r4, &r5, &r6, &r7>;
CMP_NOINLINE void sendStatic1(const Sample& s) noexcept { Static1::publish(s); }
CMP_NOINLINE void sendStatic8(const Sample& s) noexcept { Static8::publish(s); }
CMP_NOINLINE void sendStaticCancel8(const Sample& s) noexcept { StaticCancel8::publishCancelable(s); }

// B1: runtime addresses, static types; the publisher holds the wiring by value
template<class Bus>
struct Publisher {
    Bus out;
    CMP_NOINLINE void send(const Sample& s) const noexcept { out.publish(s); }
    CMP_NOINLINE void sendCancelable(const Sample& s) const noexcept { out.publishCancelable(s); }
};

// B3: a type-erased Sink<T> at a boundary
struct SinkPublisher {
    sub0::Sink<Sample> out;
    CMP_NOINLINE void send(const Sample& s) const noexcept { out.publish(s); }
};

// Hand-written: direct calls to known receivers, and a virtual loop over an array
CMP_NOINLINE void handDirect1(const Sample& s) noexcept { r0.receive(s); }
CMP_NOINLINE void handDirect8(const Sample& s) noexcept
{
    r0.receive(s); r1.receive(s); r2.receive(s); r3.receive(s);
    r4.receive(s); r5.receive(s); r6.receive(s); r7.receive(s);
}
CMP_NOINLINE void handDirectCancel8(const Sample& s) noexcept
{
    if (!gate.receive(s)) return;
    r1.receive(s); r2.receive(s); r3.receive(s); r4.receive(s); r5.receive(s); r6.receive(s); r7.receive(s);
}
CMP_NOINLINE void handVirtual(IReceiver* const* receivers, int n, const Sample& s) noexcept
{
    for (int i = 0; i < n; ++i) receivers[i]->receive(s);
}
} // namespace cmp_types

int main()
{
    using namespace cmp_types;
    const Sample msg{42};
    bench::Harness h;

    h.title("v2 StaticWiring");
    h.run(cmp::cPublish1, [&] { sendStatic1(msg); });
    h.run(cmp::cPublish8, [&] { sendStatic8(msg); });
    h.run(cmp::cCancel8, [&] { sendStaticCancel8(msg); });

    h.title("v2 wire()");
    {
        const Publisher<sub0::Wiring<NoOp>> one{sub0::wire(r0)};
        const auto eight = Publisher<sub0::Wiring<NoOp, NoOp, NoOp, NoOp, NoOp, NoOp, NoOp, NoOp>>{
            sub0::wire(r0, r1, r2, r3, r4, r5, r6, r7)};
        const auto cancel8 = Publisher<sub0::Wiring<Gate, NoOp, NoOp, NoOp, NoOp, NoOp, NoOp, NoOp>>{
            sub0::wire(gate, r1, r2, r3, r4, r5, r6, r7)};
        h.run(cmp::cPublish1, [&] { one.send(msg); });
        h.run(cmp::cPublish8, [&] { eight.send(msg); });
        h.run(cmp::cCancel8, [&] { cancel8.sendCancelable(msg); });
    }

    h.title("v2 Sink<T>");
    {
        const auto bus1 = sub0::wire(r0);
        const auto bus8 = sub0::wire(r0, r1, r2, r3, r4, r5, r6, r7);
        const SinkPublisher one{sub0::Sink<Sample>(bus1)};
        const SinkPublisher eight{sub0::Sink<Sample>(bus8)};
        h.run(cmp::cPublish1, [&] { one.send(msg); });
        h.run(cmp::cPublish8, [&] { eight.send(msg); });
    }

    h.title("Hand-written direct calls");
    h.run(cmp::cPublish1, [&] { handDirect1(msg); });
    h.run(cmp::cPublish8, [&] { handDirect8(msg); });
    h.run(cmp::cCancel8, [&] { handDirectCancel8(msg); });

    h.title("Hand-written virtual loop");
    {
        VirtualNoOp v[8];
        IReceiver* ptrs[8];
        for (int i = 0; i < 8; ++i) ptrs[i] = &v[i];
        for (auto& p : ptrs) ankerl::nanobench::doNotOptimizeAway(p);
        bench::clobberMemory();
        h.run(cmp::cPublish0, [&] { handVirtual(ptrs, 0, msg); });
        h.run(cmp::cPublish1, [&] { handVirtual(ptrs, 1, msg); });
        h.run(cmp::cPublish8, [&] { handVirtual(ptrs, 8, msg); });
    }
    return 0;
}
