// EXPECT: "pass a list of message types as sub0::Types": ForwardSubscribeAll no longer reads a std::tuple as a list
#include "sub0pub/sub0pub.hpp"
#include <tuple>
class Writer : public sub0::StreamSerializer<>, public sub0::ForwardSubscribeAll<Writer, std::tuple<int, float>>
{
public:
    explicit Writer(sub0::OStream& out) : sub0::StreamSerializer<>(out) {}
};
int main() { return sizeof(Writer) > 0 ? 0 : 1; }
