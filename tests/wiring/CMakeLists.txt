# Static wiring (sub0pub/wiring/): guarantees, limits and concurrent use

find_package(Threads REQUIRED)
add_executable(Sub0Pub_WiringTests test_wiring.cpp test_concurrent_wiring.cpp)
target_link_libraries(Sub0Pub_WiringTests PRIVATE Sub0Pub_TestMain Sub0Pub::Sub0Pub Threads::Threads)
target_include_directories(Sub0Pub_WiringTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../vendor")
add_test(NAME Sub0Pub_WiringTests COMMAND Sub0Pub_WiringTests)

# Misuse that must be rejected at compile time, with the intended diagnostic
function(sub0pub_wiring_compile_fail name regex)
    add_library(${name} OBJECT EXCLUDE_FROM_ALL compile_fail/${name}.cpp)
    target_link_libraries(${name} PRIVATE Sub0Pub::Sub0Pub)
    add_test(NAME Sub0Pub_Wiring_${name}
             COMMAND ${CMAKE_COMMAND} --build "${CMAKE_BINARY_DIR}" --target ${name} --config $<CONFIG>)
    set_tests_properties(Sub0Pub_Wiring_${name} PROPERTIES PASS_REGULAR_EXPRESSION "${regex}"
                         RESOURCE_LOCK cmake_build)
endfunction()

sub0pub_wiring_compile_fail(cf_wiring_static_wiring_local    "static storage duration|not a constant expression|not a valid template argument")
sub0pub_wiring_compile_fail(cf_wiring_publish_from_ambiguous "Origin must be bound exactly once")
sub0pub_wiring_compile_fail(cf_wiring_handles_mismatch       "Logger must receive Sample")
