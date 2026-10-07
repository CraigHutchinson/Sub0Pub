/** A station with wiring mistakes, and what the audit says about each
 *
 * Use when: an application built on publish/subscribe misbehaves quietly (a message nobody received, a receiver
 * that was never called) and you want the run itself to tell you where, or you want a test to fail when it does.
 * Demonstrates: the audit build (sub0pub_spike/audit_build.hpp named as the project header), which records every
 * publication and delivery through both the runtime broker and a StaticTo<> list and prints its findings at exit;
 * and auditFindings(), which gives a test the count.
 * Story: five mistakes, one per message type. An Alarm is raised before the audit log exists, so nobody hears it.
 * A logger is attached after the last Reading. A maintenance console subscribes to a message nothing publishes.
 * Two beacons' listeners compete for a table configured for one. And a spare status panel is constructed that the
 * Status type's StaticTo<> list does not name. None of them stops the program. The audit reports six findings:
 * the first mistake shows from both sides, as a publication nobody received and as a receiver that got nothing.
 * Keep in mind: the audit reports what this run did. It is a diagnostic build: in an ordinary build the same
 * source runs without recording anything, and the spare panel would trip a debug assertion instead.
 * Run: convergence_audit_findings returns zero when the audit holds exactly those six findings, and prints them.
 */
#include <cstdio>

#include "sub0pub_spike/topology.hpp"

struct Reading { int celsius; };
struct Alarm { int celsius; };
struct Maintenance { int ticket; };

struct Beacon
{
    int id;
    using sub0_config = sub0::config<sub0::Capacity<1>>;
};

class Panel;
extern Panel panel;

struct Status
{
    int code;
    using sub0_config = sub0::config<sub0::spike::StaticTo<&panel>>;
};

class Display final : public sub0::spike::Subscribe<Reading>
{
public:
    void receive(const Reading&) noexcept {}
};

class Logger final : public sub0::spike::Subscribe<Reading>
{
public:
    void receive(const Reading&) noexcept {}
};

class AuditLog final : public sub0::spike::Subscribe<Alarm>
{
public:
    void receive(const Alarm&) noexcept {}
};

class Console final : public sub0::spike::Subscribe<Maintenance>
{
public:
    void receive(const Maintenance&) noexcept {}
};

class Listener final : public sub0::spike::Subscribe<Beacon>
{
public:
    void receive(const Beacon&) noexcept {}
};

class Panel final : public sub0::spike::Subscribe<Status>
{
public:
    void receive(const Status&) noexcept {}
};

class Thermometer final : public sub0::spike::Publish<Reading>, public sub0::spike::Publish<Alarm>
{
public:
    void measure(int celsius) noexcept
    {
        sub0::spike::publish(*this, Reading{celsius});
        if (celsius >= 90)
            sub0::spike::publish(*this, Alarm{celsius});
    }
};

class Station final : public sub0::spike::Publish<Beacon>, public sub0::spike::Publish<Status>
{
public:
    void announce() noexcept
    {
        sub0::spike::publish(*this, Beacon{1});
        sub0::spike::publish(*this, Status{0});
    }
};

Panel panel;

int main()
{
    Display display;
    Thermometer thermometer;
    thermometer.measure(95);    // 1. the Alarm is raised before any AuditLog exists: two findings

    AuditLog auditLog;
    thermometer.measure(20);
    Logger logger;              // 2. attached after the last Reading

    Console console;            // 3. waits for a Maintenance message that nothing publishes

    Listener first;
    Listener second;            // 4. Beacon's table holds one subscriber

    Panel spare;                // 5. not in Status's StaticTo<&panel>

    Station station;
    station.announce();

    const unsigned findings = sub0::spike::auditFindings();
    std::printf("the audit holds %u findings (6 expected)\n", findings);
    return findings == 6U ? 0 : 1;
}
