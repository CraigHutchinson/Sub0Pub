/** Tests for the audit build (SUB0PUB_AUDIT): what it records through the runtime broker and through a type's
 *  StaticTo / StaticFirst list, the findings it draws from that, and how it names publishers and receivers.
 *
 * Every case uses message types of its own: a ledger is per type and lasts for the program, so a case reads the
 * report's lines about its own types and the change in the number of findings.
 */
#define SUB0PUB_AUDIT true // one translation unit: a real build defines it for all of them, from the build system
void auditTestLine(const char* line);
#define SUB0PUB_AUDIT_PRINT(line) auditTestLine(line)

#include "sub0pub/sub0pub.hpp"
#include "doctest.h"

#include <atomic>
#include <initializer_list>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// Namespace scope, so that it outlives the audit's report at exit, which is also written through auditTestLine()
static std::vector<std::string> gLines;
void auditTestLine(const char* line) { gLines.emplace_back(line); }

namespace
{
    using Report = std::vector<std::string>;

    Report report()
    {
        gLines.clear();
        (void)sub0::auditReport();
        return gLines;
    }

    /// Whether one line of the report contains every one of `parts`
    bool has(const Report& lines, std::initializer_list<const char*> parts)
    {
        for (const std::string& line : lines)
        {
            bool all = true;
            for (const char* part : parts)
                all = all && line.find(part) != std::string::npos;
            if (all)
                return true;
        }
        return false;
    }

    template<class Data>
    struct Source final : sub0::Publish<Data>
    {
        void send() noexcept { sub0::publish(*this, Data{}); }
    };

    template<class Data>
    struct Counter : sub0::Subscribe<Data>
    {
        void receive(const Data&) noexcept { ++count; }
        int count = 0;
    };

    // ---- publications nobody received ----------------------------------------------------------------------------
    struct UnheardMsg { int v = 0; };
    struct UnheardSource final : sub0::Publish<UnheardMsg>
    {
        void send() noexcept { sub0::publish(*this, UnheardMsg{}); }
    };

    struct OptionalMsg { int v = 0; using sub0_config = sub0::config<sub0::AllowNoReceivers>; };

    // ---- a full table --------------------------------------------------------------------------------------------
    struct TightMsg { int v = 0; using sub0_config = sub0::config<sub0::Capacity<1>>; };

    // ---- receivers that were never called ------------------------------------------------------------------------
    struct LateMsg { int v = 0; };
    struct LateLogger final : Counter<LateMsg> {};
    struct EarlyMsg { int v = 0; };
    struct BetweenMsg { int v = 0; };
    struct SilentMsg { int v = 0; };
    struct SilentConsole final : Counter<SilentMsg> {};

    // ---- whether a brokered type's receivers ever changed --------------------------------------------------------
    struct SteadyMsg { int v = 0; };
    struct SteadyDisplay final : Counter<SteadyMsg> {};
    struct SteadyLogger final : Counter<SteadyMsg> {};
    struct ChurnMsg { int v = 0; };

    // ---- other configurations of the runtime broker --------------------------------------------------------------
    struct SessionMsg { int v = 0; using sub0_config = sub0::config<sub0::Scoped, sub0::Snapshot>; };
    struct SessionProbe final : sub0::Subscribe<SessionMsg>
    {
        explicit SessionProbe(sub0::Domain<SessionMsg>& domain) noexcept : sub0::Subscribe<SessionMsg>(domain) {}
        void receive(const SessionMsg&) noexcept { ++count; }
        int count = 0;
    };
    struct SessionSource final : sub0::Publish<SessionMsg>
    {
        explicit SessionSource(sub0::Domain<SessionMsg>& domain) noexcept : sub0::Publish<SessionMsg>(domain) {}
        void send() noexcept { sub0::publish(*this, SessionMsg{}); }
    };

    struct LockedMsg { int v = 0; using sub0_config = sub0::config<sub0::LockWith<std::mutex>>; };
    struct LockedCounter final : sub0::Subscribe<LockedMsg>
    {
        LockedCounter() noexcept { trySubscribe(); }
        ~LockedCounter() { disconnect(); }
        void receive(const LockedMsg&) noexcept { ++count; }
        std::atomic<int> count{0};
    };

    struct PortMsg { int v = 0; };
    struct PortProbe final : Counter<PortMsg> {};
    struct PlainReceiver
    {
        void receive(const PortMsg&) noexcept { ++count; }
        int count = 0;
    };

    // ---- a type that names its receivers -------------------------------------------------------------------------
    class WiredPanel;
    extern WiredPanel wiredPanel;
    struct WiredStatus { int code = 0; using sub0_config = sub0::config<sub0::StaticTo<&wiredPanel>>; };
    class WiredPanel final : public sub0::Subscribe<WiredStatus>
    {
    public:
        void receive(const WiredStatus&) noexcept { ++count; }
        int count = 0;
    };
    WiredPanel wiredPanel;

    class FirstController;
    extern FirstController firstController;
    struct FirstMsg { int v = 0; using sub0_config = sub0::config<sub0::StaticFirst<&firstController>>; };
    class FirstController final : public sub0::Subscribe<FirstMsg>
    {
    public:
        void receive(const FirstMsg&) noexcept { ++count; }
        int count = 0;
    };
    struct FirstProbe final : Counter<FirstMsg> {};
    FirstController firstController;
}

TEST_CASE("audit: a publication that reaches no receiver is a finding that names its publisher") {
    const uint32_t before = sub0::auditFindings();
    UnheardSource source;
    source.send();
    source.send();

    CHECK(sub0::auditFindings() == before + 1U); // one finding per type, however often it happened
    const Report lines = report();
    CHECK(has(lines, {"UnheardMsg", "2 of 2 publications reached no receiver", "2 by ", "UnheardSource"}));
    CHECK(has(lines, {"sub0pub audit: ", "finding"}));
}

TEST_CASE("audit: a type that allows unheard publications records them without a finding") {
    const uint32_t before = sub0::auditFindings();
    Source<OptionalMsg> source;
    source.send();

    CHECK(sub0::auditFindings() == before);
    CHECK(has(report(), {"OptionalMsg", "1 publication", "1 reached nobody, which its configuration allows"}));
}

TEST_CASE("audit: a subscription refused by a full table is a finding") {
    const uint32_t before = sub0::auditFindings();
    Counter<TightMsg> first;
    Counter<TightMsg> second;
    REQUIRE(first.isSubscribed());
    REQUIRE_FALSE(second.isSubscribed());
    Source<TightMsg> source;
    source.send();

    CHECK(sub0::auditFindings() == before + 1U);
    const Report lines = report();
    CHECK(has(lines, {"TightMsg", "1 subscription refused: its table of 1 was full"}));
    CHECK(has(lines, {"TightMsg", "runtime table peaked at 1 of 1"}));
    CHECK_FALSE(has(lines, {"TightMsg", "candidate"})); // no advice for a type that turned a subscriber away
}

TEST_CASE("audit: a subscriber that joined after the last publication never received it") {
    const uint32_t before = sub0::auditFindings();
    Counter<LateMsg> display;
    Source<LateMsg> source;
    source.send();
    LateLogger logger;

    CHECK(sub0::auditFindings() == before + 1U);
    // It was named when the findings were asked for, without ever being called
    CHECK(has(report(), {"LateMsg", "LateLogger", "never received it: it subscribed after the last publication"}));
    CHECK(display.count == 1);
}

TEST_CASE("audit: a subscriber that left before the first publication never received it") {
    const uint32_t before = sub0::auditFindings();
    Counter<EarlyMsg> display;
    {
        Counter<EarlyMsg> early;
    }
    Source<EarlyMsg> source;
    source.send();

    CHECK(sub0::auditFindings() == before + 1U);
    CHECK(has(report(), {"EarlyMsg", "never received it: it left before the first publication"}));
}

TEST_CASE("audit: a subscriber that came and went between two publications never received it") {
    const uint32_t before = sub0::auditFindings();
    Counter<BetweenMsg> display;
    Source<BetweenMsg> source;
    source.send();
    {
        Counter<BetweenMsg> visitor;
    }
    source.send();

    CHECK(sub0::auditFindings() == before + 1U);
    CHECK(has(report(), {"BetweenMsg", "it was subscribed only between publications 1 and 2"}));
    CHECK(display.count == 2);
}

TEST_CASE("audit: a type that is subscribed to and never published is a finding") {
    const uint32_t before = sub0::auditFindings();
    SilentConsole console;

    CHECK(sub0::auditFindings() == before + 1U);
    const Report lines = report();
    CHECK(has(lines, {"SilentMsg", "never published, although 1 receiver subscribed to it"}));
    CHECK(has(lines, {"SilentConsole", "0 deliveries"}));
}

TEST_CASE("audit: a brokered type whose receivers never changed is a candidate for StaticTo") {
    const uint32_t before = sub0::auditFindings();
    SteadyDisplay display;
    SteadyLogger logger;
    Source<SteadyMsg> source;
    source.send();
    source.send();
    source.send();

    CHECK(sub0::auditFindings() == before); // nothing wrong with it
    const Report lines = report();
    CHECK(has(lines, {"SteadyMsg", "3 publications", "runtime table peaked at 2 of 8"}));
    CHECK(has(lines, {"published by ", "Source<", "SteadyMsg", ": 3"}));
    CHECK(has(lines, {"1. ", "SteadyDisplay", "3 deliveries"}));
    CHECK(has(lines, {"2. ", "SteadyLogger", "3 deliveries"}));
    CHECK(has(lines, {"its receivers never changed: a candidate for sub0::StaticTo"}));
}

TEST_CASE("audit: a brokered type whose receivers came and went is not a candidate") {
    Counter<ChurnMsg> display;
    Source<ChurnMsg> source;
    source.send();
    {
        Counter<ChurnMsg> probe;
        source.send();
    }
    source.send();

    const Report lines = report();
    CHECK(has(lines, {"1 delivery", "joined after publication 1", "left after publication 2"}));
    CHECK(has(lines, {"receivers joined or left while it was being published: keep it brokered"}));
}

TEST_CASE("audit: a StaticTo type records its listed receivers by name and reports a subscriber the list forgot") {
    const uint32_t before = sub0::auditFindings();
    Source<WiredStatus> source;
    source.send();
    source.send();
    CHECK(wiredPanel.count == 2);
    CHECK(sub0::auditFindings() == before);

    WiredPanel spare; // not in the type's list: never called
    source.send();

    CHECK(spare.count == 0);
    CHECK(sub0::auditFindings() == before + 1U);
    const Report lines = report();
    CHECK(has(lines, {"WiredStatus", "subscribes to it but is not in its StaticTo list, so it is never called"}));
    CHECK(has(lines, {"wiredPanel (", "WiredPanel)", "3 deliveries, wired"}));
    CHECK(has(lines, {"0 deliveries, not listed"}));
}

TEST_CASE("audit: a StaticFirst type counts its listed receivers, so a publication with no runtime subscriber is heard") {
    const uint32_t before = sub0::auditFindings();
    Source<FirstMsg> source;
    source.send();
    {
        FirstProbe probe;
        source.send();
        CHECK(probe.count == 1);
    }

    CHECK(firstController.count == 2);
    CHECK(sub0::auditFindings() == before);
    const Report lines = report();
    CHECK(has(lines, {"firstController (", "FirstController)", "2 deliveries, wired"}));
    CHECK(has(lines, {"FirstProbe", "1 delivery", "joined after publication 1"}));
}

TEST_CASE("audit: a Scoped type with Snapshot dispatch is recorded through its Domain") {
    const uint32_t before = sub0::auditFindings();
    sub0::Domain<SessionMsg> session;
    SessionProbe probe(session);
    SessionSource source(session);
    source.send();
    source.send();

    CHECK(probe.count == 2);
    CHECK(sub0::auditFindings() == before);
    const Report lines = report();
    CHECK(has(lines, {"SessionMsg", "2 publications"}));
    CHECK(has(lines, {"SessionProbe", "2 deliveries"}));
    CHECK(has(lines, {"published by ", "SessionSource", ": 2"}));
}

TEST_CASE("audit: concurrent publishers of a locked type are all recorded") {
    constexpr int cPerThread = 500;
    LockedCounter counter;
    REQUIRE(counter.isSubscribed());
    {
        std::thread a([] { Source<LockedMsg> source; for (int i = 0; i < cPerThread; ++i) source.send(); });
        std::thread b([] { Source<LockedMsg> source; for (int i = 0; i < cPerThread; ++i) source.send(); });
        a.join();
        b.join();
    }

    CHECK(counter.count.load() == 2 * cPerThread);
    const Report lines = report();
    CHECK(has(lines, {"LockedMsg", "1000 publications"}));
    CHECK(has(lines, {"LockedCounter", "1000 deliveries"}));
}

TEST_CASE("audit: a BrokerPort in an explicit wiring is the publisher of what it forwards") {
    const uint32_t before = sub0::auditFindings();
    PlainReceiver plain;
    sub0::BrokerPort<PortMsg> port;
    PortProbe probe;
    auto wiring = sub0::wire(plain, port);
    wiring.publish(PortMsg{});

    CHECK(plain.count == 1);
    CHECK(probe.count == 1);
    CHECK(sub0::auditFindings() == before);
    CHECK(has(report(), {"published by ", "BrokerPort<", "PortMsg", ": 1"}));
}
