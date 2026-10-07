/** Tests for a Data type's topology (sub0::StaticTo, sub0::StaticFirst): the same Subscribe / Publish / publish()
 *  source delivered by direct calls, by the runtime broker, or by both.
 *
 * The receivers a StaticTo or StaticFirst list names have static storage; every type here is unique to this
 * translation unit (anonymous namespace). The unlisted-receiver check is switched on and its action counts
 * instead of aborting.
 */
#define SUB0PUB_UNLISTED_CHECK true
namespace { int gUnlisted = 0; }
#define SUB0PUB_UNLISTED_RECEIVER(what) ((void)(what), ++gUnlisted)

#include <type_traits>

#include "doctest.h"
#include "sub0pub/sub0pub.hpp"

namespace {

int gSequence = 0; ///< orders deliveries across receivers

// ---- StaticTo: two types, three receivers, configured beside each type ------------------------------------------
class Display;
class Audit;
extern Display display;
extern Audit audit;

struct Reading
{
    int celsius;
    using sub0_config = sub0::config<sub0::StaticTo<&display, &audit>>; // member alias; this is the delivery order
};

struct Alarm
{
    int code;
    using sub0_config = sub0::config<sub0::StaticTo<&audit>>;
};

class Display final : public sub0::Subscribe<Reading>
{
public:
    void receive(const Reading& reading) noexcept
    {
        last = reading.celsius;
        order = ++gSequence;
        ++count;
    }
    int last = 0;
    int order = 0;
    int count = 0;
};

class Audit final : public sub0::SubscribeAll<Reading, Alarm>
{
public:
    void receive(const Reading&) noexcept
    {
        order = ++gSequence;
        ++readings;
    }
    void receive(const Alarm&) noexcept { ++alarms; }
    int order = 0;
    int readings = 0;
    int alarms = 0;
};

class Thermometer final : public sub0::Publish<Reading>, public sub0::Publish<Alarm>
{
public:
    void measure(int celsius) noexcept
    {
        sub0::publish(*this, Reading{celsius});
        if (celsius >= 90)
            sub0::publish(*this, Alarm{celsius});
    }
};

Display display;
Audit audit;

// A wired receiver is a plain class: nothing comes from its bases
static_assert(std::is_empty_v<sub0::Subscribe<Reading>>);
static_assert(std::is_empty_v<sub0::Publish<Reading>>);
static_assert(!std::is_polymorphic_v<Display>);
static_assert(!std::is_polymorphic_v<Audit>);
static_assert(sizeof(Display) == 3 * sizeof(int));
static_assert(sizeof(Audit) == 3 * sizeof(int), "SubscribeAll packs its empty bases on every ABI");
static_assert(sub0::Publish<Reading>::receiverCount() == 2U, "a constant of the program");
static_assert(sub0::Publish<Alarm>::receiverCount() == 1U);

// ---- a type that cannot be edited: configured from outside --------------------------------------------------------
struct Vendor
{
    int value;
};
class VendorSink;
extern VendorSink vendorSink;
} // namespace

SUB0PUB_CONFIGURE(Vendor, sub0::StaticTo<&vendorSink>);

namespace {
class VendorSink final : public sub0::Subscribe<Vendor>
{
public:
    void receive(const Vendor& vendor) noexcept { total += vendor.value; }
    int total = 0;
};
struct VendorSource final : sub0::Publish<Vendor>
{
    void send(int value) noexcept { sub0::publish(*this, Vendor{value}); }
};
VendorSink vendorSink;

// ---- one receiver, a wired type and a brokered type ---------------------------------------------------------------
class Recorder;
extern Recorder recorder;
struct Fast
{
    int value;
    using sub0_config = sub0::config<sub0::StaticTo<&recorder>>;
};
struct Slow
{
    int value;
};
class Recorder final : public sub0::SubscribeAll<Fast, Slow>
{
public:
    void receive(const Fast& fast) noexcept { fastTotal += fast.value; }
    void receive(const Slow& slow) noexcept { slowTotal += slow.value; }
    int fastTotal = 0;
    int slowTotal = 0;
};
struct Source final : sub0::Publish<Fast>, sub0::Publish<Slow>
{
    void send(int value) noexcept
    {
        sub0::publish(*this, Fast{value});
        sub0::publish(*this, Slow{value * 10});
    }
};
Recorder recorder;

// ---- StaticFirst: one bound receiver, and whoever subscribes at run time ------------------------------------------
class Panel;
extern Panel panel;
struct Status
{
    int code;
    using sub0_config = sub0::config<sub0::StaticFirst<&panel>>;
};
class Panel final : public sub0::Subscribe<Status>
{
public:
    void receive(const Status& status) noexcept
    {
        last = status.code;
        order = ++gSequence;
        ++count;
    }
    int last = 0;
    int order = 0;
    int count = 0;
};
struct StatusSource final : sub0::Publish<Status>
{
    void send(int code) noexcept { sub0::publish(*this, Status{code}); }
};
Panel panel;

// ---- filter() on a wired type, with the type's opt-in -------------------------------------------------------------
class Gate;
extern Gate gate;
struct Sample
{
    int value;
    using sub0_config = sub0::config<sub0::Filter, sub0::StaticTo<&gate>>;
};
class Gate final : public sub0::Subscribe<Sample>
{
public:
    bool filter(const Sample& sample) noexcept { return sample.value >= 0; }
    void receive(const Sample& sample) noexcept { total += sample.value; }
    int total = 0;
};
struct SampleSource final : sub0::Publish<Sample>
{
    void send(int value) noexcept { sub0::publish(*this, Sample{value}); }
};
Gate gate;

// ---- a type compiled out: an empty list, allowed ------------------------------------------------------------------
struct Trace
{
    int value;
    using sub0_config = sub0::config<sub0::StaticTo<>, sub0::AllowNoReceivers>;
};
struct TraceSource final : sub0::Publish<Trace>
{
    void send(int value) noexcept { sub0::publish(*this, Trace{value}); }
};
static_assert(sub0::Publish<Trace>::receiverCount() == 0U);

} // namespace

TEST_CASE("topology: a StaticTo type is delivered to its listed receivers, in the order of the list") {
    Thermometer thermometer;
    const int before = display.count;
    thermometer.measure(21);
    CHECK(display.count == before + 1);
    CHECK(display.last == 21);
    CHECK(audit.readings == before + 1);
    CHECK(display.order + 1 == audit.order); // StaticTo<&display, &audit>
    CHECK(sub0::receiverCount<Reading>(thermometer) == 2U);
}

TEST_CASE("topology: one receiver takes several wired types, each along its own list") {
    Thermometer thermometer;
    const int alarms = audit.alarms;
    thermometer.measure(95); // overheats: a Reading to both receivers, an Alarm to the audit log
    CHECK(audit.alarms == alarms + 1);
    CHECK(display.last == 95);
}

TEST_CASE("topology: a type configured from outside its definition is wired the same way") {
    VendorSource source;
    source.send(4);
    source.send(5);
    CHECK(vendorSink.total == 9);
}

TEST_CASE("topology: one receiver of a wired type and a brokered type") {
    Source source;
    source.send(3);
    CHECK(recorder.fastTotal == 3);
    CHECK(recorder.slowTotal == 30);
    CHECK(sub0::receiverCount<Fast>(source) == 1U);
    CHECK(sub0::receiverCount<Slow>(source) == 1U); // registered with the broker
}

TEST_CASE("topology: StaticFirst calls its bound receiver directly, then the runtime subscribers") {
    struct Late final : sub0::Subscribe<Status>
    {
        void receive(const Status& status) noexcept
        {
            last = status.code;
            order = ++gSequence;
        }
        int last = 0;
        int order = 0;
    };

    StatusSource source;
    CHECK(panel.isSubscribed()); // by its type's list: there is no registration to lose
    CHECK(sub0::receiverCount<Status>(source) == 1U);

    source.send(1);
    CHECK(panel.last == 1);

    {
        Late late;
        CHECK(late.isSubscribed());
        CHECK(sub0::receiverCount<Status>(source) == 2U);
        source.send(2);
        CHECK(panel.last == 2);
        CHECK(late.last == 2);
        CHECK(panel.order + 1 == late.order); // bound receivers first
    }

    const int count = panel.count;
    source.send(3); // the runtime subscriber has gone; the bound receiver is still called, once
    CHECK(panel.count == count + 1);
    CHECK(sub0::receiverCount<Status>(source) == 1U);
}

TEST_CASE("topology: a bound receiver of a StaticFirst type is not registered, even when asked") {
    StatusSource source;
    CHECK(panel.trySubscribe() == sub0::SubscribeResult::Subscribed);
    const int count = panel.count;
    source.send(7);
    CHECK(panel.count == count + 1); // once: directly, not also through the table
}

TEST_CASE("topology: a wired type honours filter() when the type opts in") {
    SampleSource source;
    source.send(5);
    source.send(-3); // filtered
    source.send(2);
    CHECK(gate.total == 7);
}

TEST_CASE("topology: an empty list is a type compiled out, when the type allows it") {
    TraceSource source;
    source.send(1); // no receiver, no code, no report
    CHECK(sub0::receiverCount<Trace>(source) == 0U);
}

TEST_CASE("topology: a subscriber its type's list does not name is reported, and never called") {
    const int unlisted = gUnlisted;
    {
        Display spare;
        CHECK(gUnlisted == unlisted + 1);
        Thermometer thermometer;
        thermometer.measure(30);
        CHECK(spare.count == 0);
        CHECK(display.last == 30);
    }
    CHECK(gUnlisted == unlisted + 1);
}
