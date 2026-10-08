/** Replay sensor readings through a byte buffer — an in-memory IPC simulation
 *
 * Use when: typed messages must cross a stream boundary and become local publications again.
 * Demonstrates: StreamSerializer/StreamDeserializer with ForwardSubscribe/ForwardPublish,
 * custom in-memory stream adapters, and the default binary framing.
 * Story: the sending scope serializes two sensor readings and two heartbeats, then ends.
 * The receiving scope reads those bytes and republishes them to a display and heartbeat monitor.
 * Ending the sending scope first prevents the serializer from capturing the replay again.
 * Keep in mind: this runs in one process, not through an OS pipe. Real peers must agree on
 * byte order, struct layout and type identity; the byte buffer must outlive its reader.
 * Run: Sub0Pub_IpcPipe prints the byte count, two remote readings and two heartbeats.
 */
#include "sub0pub/sub0pub.hpp"
#include <cstdio>
#include <vector>
#include <cstring>

// --- In-memory stream adapters ---

class MemoryOStream : public sub0::utility::OStream {
public:
    std::vector<char> buffer;
    StreamSize write(const char* const data, const StreamSize count) override {
        buffer.insert(buffer.end(), data, data + count);
        return count;
    }
    void flush() override {}
};

class MemoryIStream : public sub0::utility::IStream {
    const char* ptr_;
    size_t remaining_;
public:
    MemoryIStream(const char* data, size_t size) : ptr_(data), remaining_(size) {}
    StreamSize read(char* const buf, const StreamSize count) override {
        const auto n = std::min(static_cast<size_t>(count), remaining_);
        std::memcpy(buf, ptr_, n);
        ptr_ += n;
        remaining_ -= n;
        return static_cast<StreamSize>(n);
    }
    StreamSize readline(char* const buf, const StreamSize count) override { return read(buf, count); }
    StreamSize ignore(const StreamSize count) override {
        const auto n = std::min(static_cast<size_t>(count), remaining_);
        ptr_ += n; remaining_ -= n;
        return static_cast<StreamSize>(n);
    }
    StreamSize ignore(const StreamSize count, const char) override { return ignore(count); }
    bool isEof() override { return remaining_ == 0; }
};

// --- Application types ---

struct SensorReading { float temperature; uint32_t timestamp; };
struct Heartbeat { uint32_t sequenceNumber; };

// --- Process A: publishes sensor data and heartbeats, serializes to stream ---

class ProcessASerializer : public sub0::StreamSerializer<>
                         , public sub0::ForwardSubscribe<SensorReading, ProcessASerializer>
                         , public sub0::ForwardSubscribe<Heartbeat, ProcessASerializer>
{
public:
    ProcessASerializer(sub0::OStream& out) : sub0::StreamSerializer<>(out) {}
};

class SensorSource : public sub0::Publish<SensorReading> {
public:
    void sample(float temp, uint32_t ts) { sub0::publish(this, SensorReading{temp, ts}); }
};

class HeartbeatSource : public sub0::Publish<Heartbeat> {
public:
    void tick(uint32_t seq) { sub0::publish(this, Heartbeat{seq}); }
};

// --- Process B: deserializes from stream, re-publishes locally ---

class ProcessBDeserializer : public sub0::StreamDeserializer<>
                           , public sub0::ForwardPublish<SensorReading, ProcessBDeserializer>
                           , public sub0::ForwardPublish<Heartbeat, ProcessBDeserializer>
{
public:
    ProcessBDeserializer(sub0::IStream& in) : sub0::StreamDeserializer<>(in) {}
};

class RemoteSensorDisplay : public sub0::Subscribe<SensorReading> {
public:
    void receive(const SensorReading& r) noexcept {
        std::printf("  [Remote Display] temp=%.1f, ts=%u\n", r.temperature, r.timestamp);
    }
};

class HeartbeatMonitor : public sub0::Subscribe<Heartbeat> {
public:
    void receive(const Heartbeat& hb) noexcept {
        std::printf("  [Heartbeat] seq=%u\n", hb.sequenceNumber);
    }
};

int main()
{
    std::printf("=== IPC Serialization Pipe ===\n\n");

    // --- Process A side: publish and serialize ---
    MemoryOStream wire;
    {
        ProcessASerializer serializer(wire);
        SensorSource sensor;
        HeartbeatSource heartbeat;

        std::printf("Process A publishing:\n");
        sensor.sample(22.5f, 1000);
        heartbeat.tick(1);
        sensor.sample(23.1f, 2000);
        heartbeat.tick(2);
    }

    std::printf("\nSerialized %zu bytes to wire\n\n", wire.buffer.size());

    // --- Process B side: deserialize and re-publish ---
    std::printf("Process B receiving:\n");
    RemoteSensorDisplay display;
    HeartbeatMonitor monitor;

    MemoryIStream input(wire.buffer.data(), wire.buffer.size());
    {
        ProcessBDeserializer deserializer(input);
        deserializer.open();
        while (deserializer.update()) {}
    }

    return 0;
}
