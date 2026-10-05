/** Footprint of each runtime-broker configuration option (tests/footprint/measure_footprint.py, axis scenarios)
 *
 * Each object file changes one option from one of two bases, so its cost belongs to that option alone:
 * `fp::Full` (Snapshot dispatch, thread-local publish context, filter()) or the lean library default
 * (Direct, NoContext, NoFilter), spelled out explicitly where a file uses it.
 */
#ifndef SUB0PUB_FOOTPRINT_AXIS_HPP
#define SUB0PUB_FOOTPRINT_AXIS_HPP

#include "sub0pub/broker.hpp"

namespace fp
{
    using Full = sub0::config<sub0::Snapshot, sub0::ThreadLocalContext, sub0::Filter>;
}

#endif
