/** A sensor hub reports measurements and status — multiple message types
 *
 * Use when: one component sends several message types and receivers need different subsets.
 * Demonstrates: multiple Publish<T> bases, SubscribeAll over a pack of types and over a named
 * sub0::Types list, typed receive() overloads, a std::tuple as a message type, and
 * publish(this, message) selecting the appropriate publisher base.
 * Story: SensorHub emits temperature/humidity data, a status code and a calibration pair (a
 * std::tuple). Monitor receives data and status, listed in its base; Recorder receives the hub's
 * whole named list, HubMessages; StatusLogger receives only the status. The second data report
 * still bypasses StatusLogger.
 * Keep in mind: selection is by C++ message type, not by an integer topic or inheritance
 * relationship. Each message type has its own subscription table and configuration. A std::tuple
 * is one message type; a list of types to subscribe to together is a sub0::Types<...>.
 * Run: Sub0Pub_MultiType prints both monitor paths, one status-log delivery, the calibration
 * pair and the recorder's count of four records.
 */
#include "sub0pub/sub0pub.hpp"
#include <cstdio>
#include <tuple>

// A system that publishes sensor data, status codes and a calibration pair
struct SensorData { float temperature; float humidity; };
struct StatusCode { int code; };
using Calibration = std::tuple<float, float>; // offset and gain: a std::tuple is a message type like any other

// Everything the hub publishes, named once: a list of message types, not a message
using HubMessages = sub0::Types<SensorData, StatusCode, Calibration>;

class SensorHub : public sub0::Publish<SensorData>
                , public sub0::Publish<StatusCode>
                , public sub0::Publish<Calibration>
{
public:
    void calibrate(float offset, float gain) {
        sub0::publish(this, Calibration{offset, gain});
    }
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

// A recorder of everything the hub publishes: the named list stands for its three types
class Recorder : public sub0::SubscribeAll<HubMessages> {
public:
    void receive(const SensorData&) noexcept { ++records; }
    void receive(const StatusCode&) noexcept { ++records; }
    void receive(const Calibration& calibration) noexcept {
        std::printf("  [Recorder] Calibration: offset=%.1f, gain=%.2f\n",
                    std::get<0>(calibration), std::get<1>(calibration));
        ++records;
    }
    int records = 0;
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
    Recorder recorder;      // Receives every type in HubMessages

    std::printf("Reporting sensor data:\n");
    hub.reportData(22.5f, 45.0f);

    std::printf("\nReporting status:\n");
    hub.reportStatus(200);

    std::printf("\nReporting more data (logger doesn't see this):\n");
    hub.reportData(23.0f, 44.5f);

    std::printf("\nCalibrating (a std::tuple message):\n");
    hub.calibrate(0.5f, 1.02f);

    std::printf("\nSubscribeAll::Count = %zu for Monitor, %zu for Recorder; Recorder kept %d records\n",
                Monitor::Count, Recorder::Count, recorder.records);

    return 0;
}
