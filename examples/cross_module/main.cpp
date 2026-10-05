/** Numeric readings from a shared library — retained cross-DLL experiment
 *
 * Use when: investigating a publisher in one module and subscribers in another; not a supported recipe yet.
 * Demonstrates: the historical attempt to share broker instantiations through an exported module boundary.
 * Story: the executable constructs two ReadingAccumulators and a stream forwarder, then calls
 * publishReadings() in the shared library. Three float/int pairs should reach both accumulators
 * and the forwarder if the modules truly share subscription state.
 * Keep in mind: disabled in examples/CMakeLists.txt; registry/TLS sharing, ABI compatibility and
 * safe module unloading remain unvalidated. See README.md here; separate-TU success is insufficient.
 * Run: Sub0Pub_CrossModule is currently disabled. The legacy program prints a total (18.114 if
 * all intended deliveries occur) and returns its integer part, not a success/failure test code.
 */
#include <iostream>

#include "publisher.hpp"
#include "testtypes.hpp"

int main ()
{
    ReadingAccumulator accumulators[2U]; //< N subscribers

    ReadingStreamForwarder forwarder; ///< forwarder

    publishReadings();

    std::cout << "Total : " << total << std::endl;
    return int(total);
}

