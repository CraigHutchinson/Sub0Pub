# A Data type's topology (sub0::StaticTo, sub0::StaticFirst) and the no-receivers policy: the same Subscribe /
# Publish / publish() source delivered by direct calls, by the runtime broker, or by both

find_package(Threads REQUIRED)
add_executable(Sub0Pub_TopologyTests
    test_topology.cpp
    test_no_receivers.cpp
    test_station.cpp
    test_station_receivers.cpp
)
target_link_libraries(Sub0Pub_TopologyTests PRIVATE Sub0Pub_TestMain Sub0Pub::Sub0Pub Threads::Threads)
target_include_directories(Sub0Pub_TopologyTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/../vendor")
add_test(NAME Sub0Pub_TopologyTests COMMAND Sub0Pub_TopologyTests)

# Deliberate build-contract violation: one translation unit misses a type's StaticTo. Passes only if the debug
# registry reports the disagreement and the stray publication is reported as reaching nobody
add_executable(Sub0Pub_TopologyMismatch mismatch_a.cpp mismatch_b.cpp)
target_link_libraries(Sub0Pub_TopologyMismatch PRIVATE Sub0Pub::Sub0Pub Threads::Threads)
target_compile_definitions(Sub0Pub_TopologyMismatch PRIVATE SUB0PUB_CHECK_CONFIG=true SUB0PUB_NO_RECEIVERS_CHECK=true)
add_test(NAME Sub0Pub_TopologyMismatchDetected COMMAND Sub0Pub_TopologyMismatch)

# Misuse that must be rejected at compile time, with the intended diagnostic
function(sub0pub_topology_compile_fail name regex)
    add_library(${name} OBJECT EXCLUDE_FROM_ALL compile_fail/${name}.cpp)
    target_link_libraries(${name} PRIVATE Sub0Pub::Sub0Pub)
    add_test(NAME Sub0Pub_Topology_${name}
             COMMAND ${CMAKE_COMMAND} --build "${CMAKE_BINARY_DIR}" --target ${name} --config $<CONFIG>)
    set_tests_properties(Sub0Pub_Topology_${name} PROPERTIES PASS_REGULAR_EXPRESSION "${regex}"
                         RESOURCE_LOCK cmake_build)
endfunction()

sub0pub_topology_compile_fail(cf_topology_wrong_signature        "derives from Subscribe<Data> but has no receive")
sub0pub_topology_compile_fail(cf_topology_filter_not_enabled     "declares filter\\(\\), but its Data type is not configured")
sub0pub_topology_compile_fail(cf_topology_filter_signature       "declares a filter\\(\\) the runtime broker would not call")
sub0pub_topology_compile_fail(cf_topology_no_receivers           "would reach nobody")
sub0pub_topology_compile_fail(cf_topology_incomplete_receiver    "incomplete type|undefined type|C2027")
sub0pub_topology_compile_fail(cf_topology_disconnect             "no member named|is not a member|has no member")
sub0pub_topology_compile_fail(cf_topology_override               "marked 'override'|does not override|did not override|C3668")
sub0pub_topology_compile_fail(cf_topology_report                 "no matching|cannot convert|no viable|too many arguments|C2660|C2664")
