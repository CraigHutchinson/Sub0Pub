#include "doctest.h"
#include "sub0pub/sub0pub.hpp"

#include <vector>
#include <cstring>

namespace {

// In-memory OStream for capturing serialized output
class MemoryOStream : public sub0::utility::OStream {
public:
    std::vector<char> data;

    StreamSize write(const char* const buffer, const StreamSize bufferCount) override {
        data.insert(data.end(), buffer, buffer + bufferCount);
        return bufferCount;
    }
    void flush() override {}
};

// In-memory IStream for replaying serialized data
class MemoryIStream : public sub0::utility::IStream {
public:
    const char* src;
    size_t remaining;

    MemoryIStream(const char* data, size_t size) : src(data), remaining(size) {}

    StreamSize read(char* const buffer, const StreamSize bufferCount) override {
        const size_t toRead = std::min(static_cast<size_t>(bufferCount), remaining);
        std::memcpy(buffer, src, toRead);
        src += toRead;
        remaining -= toRead;
        return static_cast<StreamSize>(toRead);
    }

    StreamSize readline(char* const buffer, const StreamSize bufferCount) override {
        return read(buffer, bufferCount);
    }

    StreamSize ignore(const StreamSize bufferCount) override {
        const size_t toSkip = std::min(static_cast<size_t>(bufferCount), remaining);
        src += toSkip;
        remaining -= toSkip;
        return static_cast<StreamSize>(toSkip);
    }

    StreamSize ignore(const StreamSize bufferCount, const char) override {
        return ignore(bufferCount);
    }

    bool isEof() override { return remaining == 0; }
};

struct CountingPublisher : sub0::IPublish {
    int calls = 0;
    void publish() override { ++calls; }
};

// Serializer: subscribes to int and float, writes to stream
class TestSerializer : public sub0::StreamSerializer<>
                     , public sub0::ForwardSubscribe<int, TestSerializer>
                     , public sub0::ForwardSubscribe<float, TestSerializer>
{
public:
    TestSerializer(sub0::OStream& out) : sub0::StreamSerializer<>(out) {}
};

// Deserializer: reads from stream, publishes int and float
class TestDeserializer : public sub0::StreamDeserializer<>
                       , public sub0::ForwardPublish<int, TestDeserializer>
                       , public sub0::ForwardPublish<float, TestDeserializer>
{
public:
    TestDeserializer(sub0::IStream& in) : sub0::StreamDeserializer<>(in) {}
};

struct IntPublisher : sub0::Publish<int> {
    void send(int v) { sub0::publish(this, v); }
};

struct FloatPublisher : sub0::Publish<float> {
    void send(float v) { sub0::publish(this, v); }
};

struct IntReceiver : sub0::Subscribe<int> {
    int lastValue = 0;
    int count = 0;
    void receive(const int& v) noexcept override { lastValue = v; ++count; }
};

struct FloatReceiver : sub0::Subscribe<float> {
    float lastValue = 0.0f;
    int count = 0;
    void receive(const float& v) noexcept override { lastValue = v; ++count; }
};

// Deserializer that only knows about int (not float) — for unknown typeId test
class IntOnlyDeserializer : public sub0::StreamDeserializer<>
                          , public sub0::ForwardPublish<int, IntOnlyDeserializer>
{
public:
    IntOnlyDeserializer(sub0::IStream& in) : sub0::StreamDeserializer<>(in) {}
};

// ForwardSubscribeAll / ForwardPublishAll variants
class AllSerializer : public sub0::StreamSerializer<>
                    , public sub0::ForwardSubscribeAll<AllSerializer, int, float>
{
public:
    AllSerializer(sub0::OStream& out) : sub0::StreamSerializer<>(out) {}
};

class AllDeserializer : public sub0::StreamDeserializer<>
                      , public sub0::ForwardPublishAll<AllDeserializer, int, float>
{
public:
    AllDeserializer(sub0::IStream& in) : sub0::StreamDeserializer<>(in) {}
};

} // namespace

TEST_CASE("Serialization round-trip: int") {
    MemoryOStream outStream;

    // Phase 1: Serialize (receiver not alive yet)
    {
        IntPublisher pub;
        TestSerializer serializer(outStream);
        pub.send(42);
        pub.send(-7);
    }

    CHECK(outStream.data.size() > 0);

    // Phase 2: Deserialize and re-publish (fresh receiver)
    IntReceiver receiver;
    MemoryIStream inStream(outStream.data.data(), outStream.data.size());
    {
        TestDeserializer deserializer(inStream);
        deserializer.open();
        while (deserializer.update()) {}
    }

    CHECK(receiver.count == 2);
    CHECK(receiver.lastValue == -7);
}

TEST_CASE("Serialization round-trip: multiple types") {
    MemoryOStream outStream;

    // Phase 1: Serialize (no receivers alive)
    {
        IntPublisher intPub;
        FloatPublisher floatPub;
        TestSerializer serializer(outStream);

        intPub.send(100);
        floatPub.send(3.14f);
        intPub.send(200);
    }

    CHECK(outStream.data.size() > 0);

    // Phase 2: Deserialize with fresh receivers
    IntReceiver intReceiver;
    FloatReceiver floatReceiver;
    MemoryIStream inStream(outStream.data.data(), outStream.data.size());
    {
        TestDeserializer deserializer(inStream);
        deserializer.open();
        while (deserializer.update()) {}
    }

    CHECK(intReceiver.count == 2);
    CHECK(intReceiver.lastValue == 200);
    CHECK(floatReceiver.count == 1);
    CHECK(floatReceiver.lastValue == doctest::Approx(3.14f));
}

TEST_CASE("Type hash uniqueness") {
    const auto intHash = sub0::utility::typeHash<int>();
    const auto floatHash = sub0::utility::typeHash<float>();
    const auto doubleHash = sub0::utility::typeHash<double>();

    CHECK(intHash != floatHash);
    CHECK(intHash != doubleHash);
    CHECK(floatHash != doubleHash);
}

TEST_CASE("DefaultSerialisation protocol structure") {
    // Verify that the serialized output has the expected framing
    MemoryOStream outStream;

    {
        TestSerializer serializer(outStream);
        IntPublisher pub;
        pub.send(42);
    }

    // Expected: Prefix(4 bytes 'SUB0') + Header(8 bytes) + Data(4 bytes int) + Postfix(1 byte '\n')
    const size_t expectedSize = sizeof(sub0::DefaultSerialisation::Prefix)
                              + sizeof(sub0::DefaultSerialisation::Header)
                              + sizeof(int)
                              + sizeof(sub0::DefaultSerialisation::Postfix);
    CHECK(outStream.data.size() == expectedSize);

    // Check magic prefix
    uint32_t magic = 0;
    std::memcpy(&magic, outStream.data.data(), sizeof(magic));
    CHECK(magic == sub0::utility::FourCC<'S', 'U', 'B', '0'>::value);

    // Check postfix delimiter
    CHECK(outStream.data.back() == '\n');
}

TEST_CASE("Default serializer update has no pending work") {
    MemoryOStream outStream;
    sub0::StreamSerializer<> serializer(outStream);
    CHECK(serializer.open());
    CHECK(serializer.update());
    CHECK(outStream.data.empty());
    serializer.receive(42);
    CHECK(serializer.update());
    CHECK(outStream.data.size() == sizeof(sub0::DefaultSerialisation::Prefix)
                                + sizeof(sub0::DefaultSerialisation::Header)
                                + sizeof(int)
                                + sizeof(sub0::DefaultSerialisation::Postfix));
}

TEST_CASE("BinaryReader publishes frames without a postfix") {
    using Protocol = sub0::DefaultSerialisation;
    MemoryOStream outStream;
    sub0::BinaryWriter<Protocol::Prefix, Protocol::Header, void> writer;
    REQUIRE(writer.write(outStream, 42));

    CountingPublisher publisher;
    int data = 0;
    MemoryIStream inStream(outStream.data.data(), outStream.data.size());
    sub0::BinaryReader<Protocol::Prefix, Protocol::Header, void> reader;
    reader.setDataPublisher(data, publisher);
    REQUIRE(reader.open(inStream));
    while (reader.update(inStream)) {}
    CHECK(data == 42);
    CHECK(publisher.calls == 1);
}

#if __cpp_exceptions
TEST_CASE("BinaryReader recovers after a corrupt prefix or postfix") {
    using Protocol = sub0::DefaultSerialisation;
    const size_t frameSize = sizeof(Protocol::Prefix) + sizeof(Protocol::Header)
                           + sizeof(int) + sizeof(Protocol::Postfix);

    for (const size_t corruptOffset : {size_t(0), frameSize - 1}) {
        MemoryOStream outStream;
        Protocol::Writer writer;
        REQUIRE(writer.write(outStream, 42));
        REQUIRE(writer.write(outStream, 99));
        outStream.data[corruptOffset] = '!';

        CountingPublisher publisher;
        int data = 0;
        MemoryIStream inStream(outStream.data.data(), outStream.data.size());
        Protocol::Reader reader;
        reader.setDataPublisher(data, publisher);
        REQUIRE(reader.open(inStream));

        bool threw = false;
        try {
            while (reader.update(inStream)) {}
        } catch (const std::runtime_error&) {
            threw = true;
        }
        REQUIRE(threw);
        while (reader.update(inStream)) {}
        CHECK(publisher.calls == 1);
        CHECK(data == 99);
    }
}
#endif

// --- IPC error path tests ---

TEST_CASE("IPC: unknown typeId is skipped gracefully") {
    MemoryOStream outStream;

    // Serialize int and float
    {
        IntPublisher intPub;
        FloatPublisher floatPub;
        TestSerializer serializer(outStream);
        intPub.send(42);
        floatPub.send(3.14f); // float is unknown to IntOnlyDeserializer
        intPub.send(99);
    }

    // Deserialize with a reader that only knows int — float should be skipped
    IntReceiver receiver;
    MemoryIStream inStream(outStream.data.data(), outStream.data.size());
    {
        IntOnlyDeserializer deserializer(inStream);
        deserializer.open();
        while (deserializer.update()) {}
    }

    // Should receive both ints, skipping the unknown float
    CHECK(receiver.count == 2);
    CHECK(receiver.lastValue == 99);
}

TEST_CASE("IPC: corrupted postfix triggers error handling") {
    MemoryOStream outStream;

    {
        TestSerializer serializer(outStream);
        IntPublisher pub;
        pub.send(42);
    }

    // Corrupt the postfix byte (last byte)
    outStream.data.back() = 'X'; // was '\n'

    IntReceiver receiver;
    MemoryIStream inStream(outStream.data.data(), outStream.data.size());

    // Should either throw or silently fail depending on exception support
#if __cpp_exceptions
    bool threw = false;
    try {
        TestDeserializer deserializer(inStream);
        deserializer.open();
        while (deserializer.update()) {}
    } catch (const std::runtime_error&) {
        threw = true;
    }
    CHECK(threw);
#else
    // Without exceptions, just verify it doesn't crash
    TestDeserializer deserializer(inStream);
    deserializer.open();
    deserializer.update();
    CHECK(receiver.count == 0); // Should not have published
#endif
}

TEST_CASE("IPC: corrupted magic prefix triggers error handling") {
    MemoryOStream outStream;

    {
        TestSerializer serializer(outStream);
        IntPublisher pub;
        pub.send(42);
    }

    // Corrupt the magic prefix (first 4 bytes)
    outStream.data[0] = 'B';
    outStream.data[1] = 'A';
    outStream.data[2] = 'D';
    outStream.data[3] = '!';

    IntReceiver receiver;
    MemoryIStream inStream(outStream.data.data(), outStream.data.size());

#if __cpp_exceptions
    bool threw = false;
    try {
        TestDeserializer deserializer(inStream);
        deserializer.open();
        while (deserializer.update()) {}
    } catch (const std::runtime_error&) {
        threw = true;
    }
    CHECK(threw);
#else
    TestDeserializer deserializer(inStream);
    deserializer.open();
    deserializer.update();
    CHECK(receiver.count == 0);
#endif
}

// --- ForwardSubscribeAll / ForwardPublishAll tests ---

TEST_CASE("ForwardSubscribeAll/ForwardPublishAll round-trip") {
    MemoryOStream outStream;

    // Serialize using the All variant
    {
        IntPublisher intPub;
        FloatPublisher floatPub;
        AllSerializer serializer(outStream);

        intPub.send(123);
        floatPub.send(2.718f);
    }

    CHECK(outStream.data.size() > 0);

    // Deserialize using the All variant
    IntReceiver intReceiver;
    FloatReceiver floatReceiver;
    MemoryIStream inStream(outStream.data.data(), outStream.data.size());
    {
        AllDeserializer deserializer(inStream);
        deserializer.open();
        while (deserializer.update()) {}
    }

    CHECK(intReceiver.count == 1);
    CHECK(intReceiver.lastValue == 123);
    CHECK(floatReceiver.count == 1);
    CHECK(floatReceiver.lastValue == doctest::Approx(2.718f));
}
