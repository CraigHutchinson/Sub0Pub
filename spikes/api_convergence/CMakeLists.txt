cmake_minimum_required(VERSION 3.21)
project(Sub0PubApiConvergenceSpikes LANGUAGES CXX)

# Review spikes for converging the static and dynamic APIs (README.md). Each candidate is one weather-station
# example built in several delivery modes from shared sources; a mode contributes only examples/<candidate>/mode/<mode>.
# The A/B measurements are not built here: tools/shootout.py links the cases/ variants itself (WORKFLOW.md).

enable_testing()
find_package(Threads REQUIRED)

add_subdirectory(../.. sub0pub)

if(MSVC)
    add_compile_options(/W4 /Zc:preprocessor)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_library(convergence_spike INTERFACE)
target_include_directories(convergence_spike INTERFACE include examples/common)
target_link_libraries(convergence_spike INTERFACE Sub0Pub::Sub0Pub Threads::Threads)

# convergence_station(<candidate> MODES <mode>... [SOURCES <file>...]): one executable and test per mode
function(convergence_station candidate)
    cmake_parse_arguments(ARG "" "" "MODES;SOURCES" ${ARGN})
    set(sources examples/${candidate}/main.cpp)
    foreach(source IN LISTS ARG_SOURCES)
        list(APPEND sources examples/${candidate}/${source})
    endforeach()
    foreach(mode IN LISTS ARG_MODES)
        set(target convergence_${candidate}_${mode})
        add_executable(${target} ${sources})
        target_include_directories(${target} PRIVATE examples/${candidate}/mode/${mode} examples/${candidate})
        target_link_libraries(${target} PRIVATE convergence_spike)
        add_test(NAME ${target} COMMAND ${target})
    endforeach()
endfunction()

convergence_station(status_quo MODES dynamic static)
convergence_station(bus_parameter MODES dynamic static wired)
convergence_station(port_member MODES dynamic static)
convergence_station(typed_route MODES dynamic static hot_path bridged foreign report SOURCES thermometer.cpp)

# The first round of spikes, kept for reference: member-bound subscriptions, a member-owned wiring and a
# preprocessor-selected flow. README.md says what superseded each.
foreach(mode IN ITEMS dynamic static runtime_bound)
    add_executable(earlier_shared_flow_${mode} examples/earlier/shared_flow.cpp)
    target_link_libraries(earlier_shared_flow_${mode} PRIVATE Sub0Pub::Sub0Pub Threads::Threads)
    string(TOUPPER ${mode} mode_upper)
    target_compile_definitions(earlier_shared_flow_${mode} PRIVATE SUB0PUB_SPIKE_API=SUB0PUB_SPIKE_API_${mode_upper})
    add_test(NAME earlier_shared_flow_${mode} COMMAND earlier_shared_flow_${mode})
endforeach()

foreach(binding IN ITEMS 0 1)
    add_executable(earlier_member_subscription_${binding} examples/earlier/member_subscription.cpp)
    target_link_libraries(earlier_member_subscription_${binding} PRIVATE Sub0Pub::Sub0Pub Threads::Threads)
    target_compile_definitions(earlier_member_subscription_${binding} PRIVATE SUB0PUB_SPIKE_BINDING=${binding})
    add_test(NAME earlier_member_subscription_${binding} COMMAND earlier_member_subscription_${binding})
endforeach()

foreach(wiring_type IN ITEMS 0 1)
    add_executable(earlier_member_wiring_${wiring_type} examples/earlier/member_wiring.cpp)
    target_link_libraries(earlier_member_wiring_${wiring_type} PRIVATE Sub0Pub::Sub0Pub Threads::Threads)
    target_compile_definitions(earlier_member_wiring_${wiring_type} PRIVATE SUB0PUB_SPIKE_WIRING_TYPE=${wiring_type})
    add_test(NAME earlier_member_wiring_${wiring_type} COMMAND earlier_member_wiring_${wiring_type})
endforeach()
