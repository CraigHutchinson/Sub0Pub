/** The shared-library side of the cross-DLL experiment
 *
 * Use when: following where the module-side readings originate; start with main.cpp for the full story.
 * Demonstrates: an exported entry point invoking a publisher inside a shared library.
 * Story: publishReadings() constructs a ReadingSource and emits three float/int pairs. The
 * receivers live in the executable, so delivery depends on shared broker state across the boundary.
 * Keep in mind: this retained experiment is disabled and does not establish portable DLL support.
 * Run: part of the disabled Sub0Pub_CrossModule_Publisher target, called by Sub0Pub_CrossModule.
 */
#include "testtypes.hpp"
#include "publisher.hpp"


void publishReadings()
{
    ReadingSource source;

    for ( uint32_t i = 0U; i < 3U; ++i )
    {
        source.publishPair();
    }
}
