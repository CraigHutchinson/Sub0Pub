/** The export boundary of the cross-DLL experiment
 *
 * Use when: inspecting the historical broker-sharing approach; main.cpp explains the intended deliveries.
 * Demonstrates: platform export/import annotations and explicit BrokerImpl template instantiations.
 * Story: this header declares publishReadings() and tries to expose the float/int broker state
 * used by the shared-library publisher and executable receivers.
 * Keep in mind: BrokerImpl is internal API. Export declarations alone do not prove shared state,
 * shared dispatch context or safe unloading; this is disabled pending a supported public boundary.
 * Run: companion header for the disabled Sub0Pub_CrossModule targets; not a standalone sample.
 */
#pragma once

#include "sub0pub/sub0pub.hpp"

// Cross-platform shared library export/import macros
#if defined(_WIN32)
    #if Sub0Pub_CrossModule_Publisher_EXPORTS
        #define SUB0PUB_EXAMPLE_API __declspec(dllexport)
        #define SUB0PUB_EXAMPLE_EXTERN
    #else
        #define SUB0PUB_EXAMPLE_API __declspec(dllimport)
        #define SUB0PUB_EXAMPLE_EXTERN extern
    #endif
#elif defined(__GNUC__) || defined(__clang__)
    #define SUB0PUB_EXAMPLE_API __attribute__((visibility("default")))
    #define SUB0PUB_EXAMPLE_EXTERN
#else
    #define SUB0PUB_EXAMPLE_API
    #define SUB0PUB_EXAMPLE_EXTERN
#endif

// Historical sharing attempt: exports alone do not establish a portable registry/context contract.
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4492) // thread_local with dllexport
#endif
SUB0PUB_EXAMPLE_EXTERN template class SUB0PUB_EXAMPLE_API sub0::detail::BrokerImpl<float, sub0::config_t<float>>;
SUB0PUB_EXAMPLE_EXTERN template class SUB0PUB_EXAMPLE_API sub0::detail::BrokerImpl<int, sub0::config_t<int>>;
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

SUB0PUB_EXAMPLE_API void publishReadings();
