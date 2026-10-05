// An external project using an installed Sub0Pub: every entry header, built with warnings as errors, and each area
// used for real -- the runtime broker, the static wiring and an IPC round trip through a byte buffer
#include <sub0pub/sub0pub.hpp>
#include <sub0pub/broker.hpp>
#include <sub0pub/config.hpp>
#include <sub0pub/ipc.hpp>
#include <sub0pub/wiring.hpp>

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <utility>
#include <vector>

// A C++23 library operation also proves the exported requirement reaches the consumer translation unit.
enum class ConsumerMode { Ready = 23 };
static_assert(std::to_underlying(ConsumerMode::Ready) == 23);

namespace {

struct Reading { int value; };

struct Total final : sub0::Subscribe<Reading>
{
    int sum = 0;
    void receive(const Reading& reading) noexcept override { sum += reading.value; }
};

struct Source final : sub0::Publish<Reading>
{
    void send(int value) noexcept { sub0::publish(*this, Reading{value}); }
};

struct Display
{
    int last = 0;
    void receive(const Reading& reading) noexcept { last = reading.value; }
};

class ByteSink final : public sub0::utility::OStream
{
public:
    std::vector<char> bytes;
    StreamSize write(const char* const buffer, const StreamSize count) override
    {
        bytes.insert(bytes.end(), buffer, buffer + count);
        return count;
    }
    void flush() override {}
};

class ByteSource final : public sub0::utility::IStream
{
public:
    ByteSource(const char* data, std::size_t size) : data_(data), remaining_(size) {}
    StreamSize read(char* const buffer, const StreamSize count) override
    {
        const std::size_t n = std::min(static_cast<std::size_t>(count), remaining_);
        std::memcpy(buffer, data_, n);
        data_ += n;
        remaining_ -= n;
        return static_cast<StreamSize>(n);
    }
    StreamSize readline(char* const buffer, const StreamSize count) override { return read(buffer, count); }
    StreamSize ignore(const StreamSize count) override
    {
        const std::size_t n = std::min(static_cast<std::size_t>(count), remaining_);
        data_ += n;
        remaining_ -= n;
        return static_cast<StreamSize>(n);
    }
    StreamSize ignore(const StreamSize count, const char) override { return ignore(count); }
    bool isEof() override { return remaining_ == 0; }

private:
    const char* data_;
    std::size_t remaining_;
};

class Writer final : public sub0::StreamSerializer<>, public sub0::ForwardSubscribe<Reading, Writer>
{
public:
    explicit Writer(sub0::OStream& out) : sub0::StreamSerializer<>(out) {}
};

class Reader final : public sub0::StreamDeserializer<>, public sub0::ForwardPublish<Reading, Reader>
{
public:
    explicit Reader(sub0::IStream& in) : sub0::StreamDeserializer<>(in) {}
};

} // namespace

int main()
{
    ByteSink sent;
    {
        Source source;
        Writer writer(sent);
        source.send(2); // broker: no subscriber yet except the writer, which serializes it
    }

    Total total;
    ByteSource received(sent.bytes.data(), sent.bytes.size());
    {
        Reader reader(received);
        reader.open();
        while (reader.update()) {}
    }

    Display display;
    auto wiring = sub0::wire(display);
    wiring.publish(Reading{3});

    return total.isSubscribed() && total.sum == 2 && display.last == 3 ? 0 : 1;
}
