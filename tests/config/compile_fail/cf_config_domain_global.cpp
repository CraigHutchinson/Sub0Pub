// EXPECT: "Domain<Data> requires a Data type configured with sub0::Scoped"
#include "sub0pub/sub0pub.hpp"
struct Msg {};
int main() { sub0::Domain<Msg> d; (void)d; }
