# Per-Data configuration (sub0pub/config.hpp and sub0pub/broker/). Built with a project default configuration header,
# as a project would set it from its build system, so these TUs are a separate executable from Sub0Pub_Tests.

function(sub0pub_config_target name)
    target_link_libraries(${name} PRIVATE Sub0Pub::Sub0Pub)
    target_include_directories(${name} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/../vendor")
    target_compile_definitions(${name} PRIVATE SUB0PUB_CONFIG_HEADER="project_config.hpp" ${ARGN})
endfunction()

find_package(Threads REQUIRED)
add_executable(Sub0Pub_ConfigTests
    test_binding.cpp
    test_multi_tu_a.cpp
    test_multi_tu_b.cpp
    test_endpoints.cpp
    test_axes.cpp
)
sub0pub_config_target(Sub0Pub_ConfigTests SUB0PUB_TEST_COUNT_MISMATCHES SUB0PUB_CHECK_CONFIG=true)
target_link_libraries(Sub0Pub_ConfigTests PRIVATE Sub0Pub_TestMain Threads::Threads)
add_test(NAME Sub0Pub_ConfigTests COMMAND Sub0Pub_ConfigTests)

# Deliberate cross-TU configuration mismatch: passes only if the debug registry detects it
add_executable(Sub0Pub_ConfigMismatch mismatch_a.cpp mismatch_b.cpp)
sub0pub_config_target(Sub0Pub_ConfigMismatch SUB0PUB_TEST_COUNT_MISMATCHES SUB0PUB_CHECK_CONFIG=true)
add_test(NAME Sub0Pub_ConfigMismatchDetected COMMAND Sub0Pub_ConfigMismatch)

# Misuse must be rejected at compile time with the intended diagnostic
function(sub0pub_config_compile_fail name regex)
    add_library(${name} OBJECT EXCLUDE_FROM_ALL compile_fail/${name}.cpp)
    sub0pub_config_target(${name})
    add_test(NAME Sub0Pub_Config_${name}
             COMMAND ${CMAKE_COMMAND} --build "${CMAKE_BINARY_DIR}" --target ${name} --config $<CONFIG>)
    set_tests_properties(Sub0Pub_Config_${name} PROPERTIES PASS_REGULAR_EXPRESSION "${regex}"
                         RESOURCE_LOCK cmake_build)
endfunction()

sub0pub_config_compile_fail(cf_config_two_places          "configured in exactly one place")
sub0pub_config_compile_fail(cf_config_filter_disabled     "override|overriding|conflicting return type|different return type|C2555|C3248")
sub0pub_config_compile_fail(cf_config_cancel_no_context   "cancel\\(\\) needs a publish context")
sub0pub_config_compile_fail(cf_config_lock_direct         "a Lock requires Snapshot dispatch")
sub0pub_config_compile_fail(cf_config_lock_static_context "a Lock requires ThreadLocalContext")
sub0pub_config_compile_fail(cf_config_snapshot_no_context "Snapshot needs a publish context")
sub0pub_config_compile_fail(cf_config_domain_global       "requires a Data type configured with sub0::Scoped")
sub0pub_config_compile_fail(cf_config_scoped_needs_domain "no matching|no appropriate default constructor|deleted|default constructor")
sub0pub_config_compile_fail(cf_config_delete_via_base     "protected|inaccessible")
sub0pub_config_compile_fail(cf_config_delete_publish_via_base "protected|inaccessible")

# The same, for the Builtin default (no project configuration header): costly features are opt-in and detected
function(sub0pub_default_compile_fail name regex)
    add_library(${name} OBJECT EXCLUDE_FROM_ALL compile_fail/${name}.cpp)
    target_link_libraries(${name} PRIVATE Sub0Pub::Sub0Pub)
    add_test(NAME Sub0Pub_Config_${name}
             COMMAND ${CMAKE_COMMAND} --build "${CMAKE_BINARY_DIR}" --target ${name} --config $<CONFIG>)
    set_tests_properties(Sub0Pub_Config_${name} PROPERTIES PASS_REGULAR_EXPRESSION "${regex}"
                         RESOURCE_LOCK cmake_build)
endfunction()

sub0pub_default_compile_fail(cf_config_filter_not_enabled   "conflicting return type|different return type|overrides a 'final'|return type differs|C2555")
sub0pub_default_compile_fail(cf_config_cancel_default       "needs a publish context")

# Removed policy flags must fail rather than silently disabling an application's requested safety policy.
foreach(policy REENTRANT_SAFE CANCEL FILTER THREAD_SAFE)
    set(target cf_config_removed_${policy})
    add_library(${target} OBJECT EXCLUDE_FROM_ALL compile_fail/cf_config_removed_policy.cpp)
    target_link_libraries(${target} PRIVATE Sub0Pub::Sub0Pub)
    target_compile_definitions(${target} PRIVATE SUB0PUB_${policy}=false)
    add_test(NAME Sub0Pub_Config_${target}
             COMMAND ${CMAKE_COMMAND} --build "${CMAKE_BINARY_DIR}" --target ${target} --config $<CONFIG>)
    set_tests_properties(Sub0Pub_Config_${target} PROPERTIES
                         PASS_REGULAR_EXPRESSION "Sub0Pub policy macros were removed" RESOURCE_LOCK cmake_build)
endforeach()

# config<Opts...> over different defaults in different translation units, in both link orders
foreach(order ab ba)
    if(order STREQUAL "ab")
        set(sources identity_a.cpp identity_b.cpp)
    else()
        set(sources identity_b.cpp identity_a.cpp)
    endif()
    add_executable(Sub0Pub_ConfigIdentity_${order} ${sources} identity_main.cpp)
    target_link_libraries(Sub0Pub_ConfigIdentity_${order} PRIVATE Sub0Pub::Sub0Pub Threads::Threads)
    target_include_directories(Sub0Pub_ConfigIdentity_${order} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    # Out-of-line calls, so the linker's choice between the TUs' definitions of a merged function is observable
    target_compile_options(Sub0Pub_ConfigIdentity_${order} PRIVATE
        $<IF:$<CXX_COMPILER_ID:MSVC>,/Od /Ob0,-O0 -fno-inline>)
    add_test(NAME Sub0Pub_ConfigIdentity_${order} COMMAND Sub0Pub_ConfigIdentity_${order})
    set_tests_properties(Sub0Pub_ConfigIdentity_${order} PROPERTIES TIMEOUT 30)
endforeach()
