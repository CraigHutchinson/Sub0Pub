# The doctest runner contains no Sub0Pub headers or per-suite configuration.
# Compile it once; keep all test translation units independent for header/ODR coverage.
add_library(Sub0Pub_TestMain OBJECT test_main.cpp)
target_compile_features(Sub0Pub_TestMain PRIVATE cxx_std_23)
target_include_directories(Sub0Pub_TestMain PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/vendor")

add_executable(Sub0Pub_Tests
    test_pubsub.cpp
    test_cancel.cpp
    test_subscribe_all.cpp
    test_serialization.cpp
    test_fingerprint.cpp
    test_capacity_bounded.cpp
    test_reentrant_check.cpp
    test_thread_safe.cpp
    test_typeidname.cpp
    test_explicit_instantiation.cpp
    test_defaults.cpp
)

find_package(Threads REQUIRED)
target_link_libraries(Sub0Pub_Tests PRIVATE Sub0Pub_TestMain Sub0Pub::Sub0Pub Threads::Threads)

target_include_directories(Sub0Pub_Tests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor"
)

add_test(NAME Sub0Pub_Tests COMMAND Sub0Pub_Tests)

# Performance benchmarks are built explicitly by the measurement job, not by routine builds or ctest.
# The core benchmark is built once per policy (tests/policy/policy.hpp, named as the project's configuration header)
# so policies are compared on identical scenarios.
function(sub0pub_add_bench name source)
    add_executable(${name} EXCLUDE_FROM_ALL ${source})
    target_link_libraries(${name} PRIVATE Sub0Pub::Sub0Pub)
    target_include_directories(${name} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/vendor")
    target_compile_definitions(${name} PRIVATE ${ARGN})
endfunction()

sub0pub_add_bench(Sub0Pub_Bench            bench/bench_core.cpp SUB0PUB_REENTRANT_CHECK=false)
sub0pub_add_bench(Sub0Pub_Bench_Checked    bench/bench_core.cpp SUB0PUB_REENTRANT_CHECK=true)
sub0pub_add_bench(Sub0Pub_Bench_Full       bench/bench_core.cpp POLICY_SNAPSHOT=1 POLICY_CONTEXT=1 POLICY_FILTER=1)
sub0pub_add_bench(Sub0Pub_Bench_ThreadSafe bench/bench_core.cpp POLICY_LOCK=1 POLICY_FILTER=1)
foreach(bench Sub0Pub_Bench Sub0Pub_Bench_Checked Sub0Pub_Bench_Full Sub0Pub_Bench_ThreadSafe)
    target_include_directories(${bench} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/policy")
    target_compile_definitions(${bench} PRIVATE SUB0PUB_CONFIG_HEADER="policy.hpp")
endforeach()
sub0pub_add_bench(Sub0Pub_Bench_Ipc        bench/bench_ipc.cpp)
sub0pub_add_bench(Sub0Pub_Bench_Axes       bench/bench_axes.cpp) # each configuration option alone (docs/DESIGN.md)

# Per-Data configuration, static wiring and a type's topology (public API: sub0pub/config.hpp, sub0pub/broker/,
# sub0pub/wiring/)
add_subdirectory(config)
add_subdirectory(wiring)
add_subdirectory(topology)
add_subdirectory(audit)

# Header hygiene: every header self-contained, each entry header isolated from the other areas
add_subdirectory(headers)

# The installed package, as an external find_package(Sub0Pub) project uses it
add_subdirectory(package)

# Collapse evidence: every variant behaves exactly like its hand-written reference (docs/EVIDENCE.md)
add_subdirectory(collapse)

# Build-time measurement is separate from runtime benchmarks and does not run by default.
add_subdirectory(compile_time)
