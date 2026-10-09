/** One log stream, different audiences — per-subscriber filtering
 *
 * Use when: receivers of the same message type need different subsets of its values.
 * Demonstrates: per-type sub0::Filter and filter() deciding whether receive() runs for that subscriber.
 * Story: Logger publishes five entries. AlertDisplay receives warnings/errors; DebugConsole
 * receives debug entries; FileLog prints every entry as a stand-in for a file sink.
 * Keep in mind: rejecting a message skips this receiver only; it does not cancel delivery to
 * other subscribers. Filtering is opt-in and still executes a predicate; no cost-free claim is made.
 * Run: Sub0Pub_Filtering prints two ALERT, two DEBUG and five FILE lines.
 */
#include "sub0pub/sub0pub.hpp"
#include <cstdio>

struct LogEntry {
    int level;      // 0=debug, 1=info, 2=warn, 3=error
    const char* msg;
    using sub0_config = sub0::config<sub0::Filter>; // filter() is opt-in: the type says so
};

class Logger : public sub0::Publish<LogEntry> {
public:
    void log(int level, const char* msg) {
        sub0::publish(this, LogEntry{level, msg});
    }
};

// Only receives warnings and errors (level >= 2)
class AlertDisplay : public sub0::Subscribe<LogEntry> {
public:
    bool filter(const LogEntry& entry) noexcept {
        return entry.level >= 2;
    }
    void receive(const LogEntry& entry) noexcept {
        std::printf("  [ALERT] level=%d: %s\n", entry.level, entry.msg);
    }
};

// Receives everything
class FileLog : public sub0::Subscribe<LogEntry> {
public:
    void receive(const LogEntry& entry) noexcept {
        std::printf("  [FILE]  level=%d: %s\n", entry.level, entry.msg);
    }
};

// Only receives debug messages
class DebugConsole : public sub0::Subscribe<LogEntry> {
public:
    bool filter(const LogEntry& entry) noexcept {
        return entry.level == 0;
    }
    void receive(const LogEntry& entry) noexcept {
        std::printf("  [DEBUG] %s\n", entry.msg);
    }
};

int main()
{
    std::printf("=== Message Filtering ===\n\n");

    Logger logger;
    AlertDisplay alerts;   // level >= 2 only
    FileLog file;          // everything
    DebugConsole debug;    // level == 0 only

    logger.log(0, "System starting up");
    logger.log(1, "Connected to server");
    logger.log(2, "Disk space low");
    logger.log(3, "Connection lost!");
    logger.log(0, "Retrying...");

    return 0;
}
