/** A station with five wiring mistakes, and what the audit says about each
 *
 * Use when: an application built on publish/subscribe misbehaves quietly (a message nobody received, a receiver
 * that was never called), or you want to know which message types could name their receivers with StaticTo.
 * Demonstrates: the audit build (SUB0PUB_AUDIT), which records every publication and delivery through the runtime
 * broker and through a StaticTo list and writes a report at exit; sub0::auditFindings(), which gives a test the
 * number of findings; and sub0::auditReport(), which writes the report on request.
 * Story: one mistake per message type. An Alarm is raised before the alarm log exists, so nobody hears it. A logger
 * subscribes after the last Reading. A maintenance console waits for a message that nothing publishes. Two
 * listeners compete for a Beacon table configured for one. A spare status panel is constructed that the Status
 * type's StaticTo list does not name. None of them stops the program. The audit reports six findings: the first
 * mistake shows from both sides, as a publication nobody received and as a receiver that got nothing. Below the
 * findings the report lists, per message type, who published it and who received it, in order.
 * Keep in mind: the audit describes this run, not every run. It is a diagnostic build: every publication takes a
 * lock, and the checks that would stop a debug build at the first mistake are off so that the run completes. An
 * application defines SUB0PUB_AUDIT for every translation unit, from its build system; this one-file program
 * defines it below. SUB0PUB_AUDIT_PRINT sends the report somewhere other than stderr.
 * Run: Sub0Pub_Example_audit_findings returns zero when the audit holds exactly those six findings, and writes
 * its report to stderr at exit.
 */
#ifndef SUB0PUB_AUDIT
#define SUB0PUB_AUDIT true
#endif

#include "sub0pub/sub0pub.hpp"
#include <cstdio>

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
    using sub0_config = sub0::config<sub0::StaticTo<&panel>>;
};

class Display final : public sub0::Subscribe<Reading>
{
public:
    void receive(const Reading&) noexcept {}
};

class Logger final : public sub0::Subscribe<Reading>
{
public:
    void receive(const Reading&) noexcept {}
};

class AlarmLog final : public sub0::Subscribe<Alarm>
{
public:
    void receive(const Alarm&) noexcept {}
};

class Console final : public sub0::Subscribe<Maintenance>
{
public:
    void receive(const Maintenance&) noexcept {}
};

class Listener final : public sub0::Subscribe<Beacon>
{
public:
    void receive(const Beacon&) noexcept {}
};

class Panel final : public sub0::Subscribe<Status>
{
public:
    void receive(const Status&) noexcept {}
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

class Station final : public sub0::Publish<Beacon>, public sub0::Publish<Status>
{
public:
    void announce() noexcept
    {
        sub0::publish(*this, Beacon{1});
        sub0::publish(*this, Status{0});
    }
};

Panel panel;

int main()
{
    Display display;
    Thermometer thermometer;
    thermometer.measure(95);    // 1. The Alarm is raised before any AlarmLog exists: two findings.

    AlarmLog alarmLog;
    thermometer.measure(20);
    Logger logger;              // 2. Subscribes after the last Reading.

    Console console;            // 3. Waits for a Maintenance message that nothing publishes.

    Listener first;
    Listener second;            // 4. Beacon's table holds one subscriber.

    Panel spare;                // 5. Not in Status's StaticTo<&panel>.

    Station station;
    station.announce();

    const unsigned findings = sub0::auditFindings();
    std::printf("the audit holds %u findings (6 expected); its report follows on stderr\n", findings);
    return findings == 6U ? 0 : 1;
}
