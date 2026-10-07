/** A sensor hub reports measurements and status — multiple message types
 *
 * Use when: one component sends several message types and receivers need different subsets.
 * Demonstrates: multiple Publish<T> bases, SubscribeAll, typed receive() overloads, and
 * publish(this, message) selecting the appropriate publisher base.
 * Story: SensorHub emits temperature/humidity data and a status code. Monitor receives both;
 * StatusLogger receives only the status. The second data report still bypasses StatusLogger.
 * Keep in mind: selection is by C++ message type, not by an integer topic or inheritance
 * relationship. Each message type has its own subscription table and configuration.
 * Run: Sub0Pub_MultiType prints both monitor paths and one status-log delivery.
 */
#include "sub0pub/sub0pub.hpp"
#include <cstdio>

// A system that publishes both sensor data and status codes
struct SensorData { float temperature; float humidity; };
struct StatusCode { int code; };

class SensorHub : public sub0::Publish<SensorData>
                , public sub0::Publish<StatusCode>
{
public:
    void reportData(float temp, float hum) {
        sub0::publish(this, SensorData{temp, hum});
    }
    void reportStatus(int code) {
        sub0::publish(this, StatusCode{code});
    }
};

// A monitor that receives both types
class Monitor : public sub0::SubscribeAll<SensorData, StatusCode> {
public:
    void receive(const SensorData& data) noexcept {
        std::printf("  [Monitor] Sensor: temp=%.1f, humidity=%.1f\n",
                    data.temperature, data.humidity);
    }
    void receive(const StatusCode& status) noexcept {
        std::printf("  [Monitor] Status: code=%d\n", status.code);
    }
};

// A logger that only cares about status codes
class StatusLogger : public sub0::Subscribe<StatusCode> {
public:
    void receive(const StatusCode& status) noexcept {
        std::printf("  [StatusLogger] Logged status code: %d\n", status.code);
    }
};

int main()
{
    std::printf("=== Multi-Type Subscribe ===\n");

    SensorHub hub;
    Monitor monitor;        // Receives both SensorData and StatusCode
    StatusLogger logger;    // Receives only StatusCode

    std::printf("Reporting sensor data:\n");
    hub.reportData(22.5f, 45.0f);

    std::printf("\nReporting status:\n");
    hub.reportStatus(200);

    std::printf("\nReporting more data (logger doesn't see this):\n");
    hub.reportData(23.0f, 44.5f);

    std::printf("\nSubscribeAll::Count = %zu\n", Monitor::Count);

    return 0;
}
