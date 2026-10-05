/** Footprint: 1 type, 1 publisher, 1 subscriber, 1 publish site (Full, Implementation=SingleSubscriberBroker) */
#include "fp_axis.hpp"

/// The Implementation<> hook's worked example (test_endpoints.cpp): one subscriber, one pointer of RAM
template<class Data, class Config>
class SingleSubscriberBroker
{
public:
    sub0::SubscribeResult trySubscribe(sub0::Subscribe<Data>* s) noexcept
    {
        if (slot() != nullptr)
            return sub0::SubscribeResult::CapacityExceeded;
        slot() = s;
        return sub0::SubscribeResult::Subscribed;
    }
    void disconnect(sub0::Subscribe<Data>* s) noexcept
    {
        if (slot() == s)
            slot() = nullptr;
        sub0::kit::forgetInOwnDispatches<Data>(&slot(), s);
    }
    void publish(const Data& data, const void* origin, sub0::PublishReport* report) const noexcept
    {
        sub0::Subscribe<Data>* snapshot[1] = { slot() };
        sub0::kit::DispatchScope<Data> scope(&slot(), origin, report, snapshot, 1);
        sub0::kit::deliverAt<Data>(snapshot[0], data);
    }
    void cancel() const noexcept { sub0::kit::cancel<Data>(&slot()); }
private:
    static sub0::Subscribe<Data>*& slot() noexcept { static sub0::Subscribe<Data>* s = nullptr; return s; }
};

struct MsgA { int value; using sub0_config = sub0::with<fp::Full, sub0::Implementation<SingleSubscriberBroker>>; };

struct SinkA : sub0::Subscribe<MsgA> { MsgA last{}; void receive(const MsgA& d) noexcept override { last = d; } };
struct SourceA : sub0::Publish<MsgA> { void send(const MsgA& d) noexcept { sub0::publish(*this, d); } };

SourceA gSourceA;
SinkA gSinkA;

extern "C" void fp_publish_a(int v) noexcept { gSourceA.send(MsgA{v}); }

extern "C" { char fp_sizeof_Subscribe[sizeof(sub0::Subscribe<MsgA>)]; char fp_sizeof_Publish[sizeof(sub0::Publish<MsgA>)]; }
