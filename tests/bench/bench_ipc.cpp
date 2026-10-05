/** End-to-end IPC benchmarks: publish -> StreamSerializer -> OStream, IStream -> StreamDeserializer -> subscriber
 *
 * Streams are in-memory and pre-sized so the numbers measure Sub0Pub's framing, dispatch and copy cost
 * rather than a transport. A memcpy of the same framed size is included as the floor.
 */
#define ANKERL_NANOBENCH_IMPLEMENT
#include "sub0pub/sub0pub.hpp"

#include "bench_harness.hpp"
#include "bench_system_info.hpp"

#include <array>
#include <cstring>
#include <vector>

namespace {

template<std::size_t N>
struct Payload { std::array<char, N> bytes; };

/// Fixed-capacity in-memory output stream; rewind() between iterations
class MemoryOStream : public sub0::OStream {
public:
    explicit MemoryOStream(std::size_t capacity) : data_(capacity) {}

    StreamSize write(const char* const buffer, const StreamSize bufferCount) override
    {
        std::memcpy(data_.data() + size_, buffer, bufferCount);
        size_ += bufferCount;
        return bufferCount;
    }
    void flush() override {}

    void rewind() noexcept { size_ = 0; }
    std::size_t size() const noexcept { return size_; }
    const char* data() const noexcept { return data_.data(); }

private:
    std::vector<char> data_;
    std::size_t size_ = 0;
};

/// In-memory input stream over a fixed buffer; rewind() replays it
class MemoryIStream : public sub0::IStream {
public:
    MemoryIStream(const char* data, std::size_t size) : begin_(data), src_(data), end_(data + size) {}

    StreamSize read(char* const buffer, const StreamSize bufferCount) override
    {
        const std::size_t n = std::min<std::size_t>(bufferCount, static_cast<std::size_t>(end_ - src_));
        std::memcpy(buffer, src_, n);
        src_ += n;
        return static_cast<StreamSize>(n);
    }
    StreamSize readline(char* const buffer, const StreamSize bufferCount) override { return read(buffer, bufferCount); }
    StreamSize ignore(const StreamSize bufferCount) override
    {
        const std::size_t n = std::min<std::size_t>(bufferCount, static_cast<std::size_t>(end_ - src_));
        src_ += n;
        return static_cast<StreamSize>(n);
    }
    StreamSize ignore(const StreamSize bufferCount, const char) override { return ignore(bufferCount); }
    bool isEof() override { return src_ == end_; }

    void rewind() noexcept { src_ = begin_; }

private:
    const char* begin_;
    const char* src_;
    const char* end_;
};

template<typename Data>
struct Source : sub0::Publish<Data> {
    void send(const Data& d) noexcept { sub0::publish(*this, d); }
};

template<typename Data>
struct Sink : sub0::Subscribe<Data> {
    uint32_t count = 0;
    void receive(const Data&) noexcept override { ++count; }
};

template<typename Data>
class Serializer : public sub0::StreamSerializer<>
                 , public sub0::ForwardSubscribe<Data, Serializer<Data>> {
public:
    explicit Serializer(sub0::OStream& out) : sub0::StreamSerializer<>(out) {}
};

template<typename Data>
class Deserializer : public sub0::StreamDeserializer<>
                   , public sub0::ForwardPublish<Data, Deserializer<Data>> {
public:
    explicit Deserializer(sub0::IStream& in) : sub0::StreamDeserializer<>(in) {}
};

template<std::size_t N>
void runPayload(bench::Harness& h)
{
    using Data = Payload<N>;
    const std::string size = std::to_string(N) + "B payload";

    // Capture one framed message to size the streams and the memcpy floor
    MemoryOStream out(64 + N);
    std::size_t framed = 0;
    {
        Source<Data> src;
        Serializer<Data> serializer(out);
        src.send(Data{});
        framed = out.size();
    }

    h.title("IPC " + size + " (" + std::to_string(framed) + "B framed)");

    {
        Source<Data> src;
        Serializer<Data> serializer(out);
        const Data d{};
        h.run("serialize: publish -> stream", [&] {
            out.rewind();
            src.send(d);
        });
    }

    {
        std::vector<char> wire(out.data(), out.data() + framed);
        MemoryIStream in(wire.data(), wire.size());
        Sink<Data> sink;
        Deserializer<Data> deserializer(in);
        deserializer.open();
        // @note update() returns true once a frame prefix is read, not on publish, so completion is
        //       detected by delivery to the sink rather than by update()'s return value.
        h.run("deserialize: stream -> subscriber", [&] {
            in.rewind();
            const uint32_t before = sink.count;
            while (sink.count == before && !in.isEof())
                deserializer.update();
            if (sink.count == before) // stream ended mid-frame: a trailing update() completes it
                deserializer.update();
        });
        if (sink.count == 0)
            std::cerr << "deserialize produced no messages for " << size << std::endl;
    }

    {
        std::vector<char> src(framed), dst(framed);
        h.run("floor: memcpy framed message", [&] {
            std::memcpy(dst.data(), src.data(), framed);
            ankerl::nanobench::doNotOptimizeAway(dst.data());
        });
    }
}

} // namespace

int main()
{
    printSystemInfo();
    bench::Harness h;
    runPayload<4>(h);
    runPayload<64>(h);
    runPayload<256>(h);
    return 0;
}
