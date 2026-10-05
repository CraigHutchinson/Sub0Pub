add_subdirectory(basic_pubsub)
add_subdirectory(multi_type)
add_subdirectory(filtering)
add_subdirectory(cancellation)
add_subdirectory(ipc_pipe)
add_subdirectory(layout_check)
add_subdirectory(minimal_sub0pub)

# cross_module disabled: cross-module (DLL / shared library) use is not supported yet (cross_module/README.md).
# add_subdirectory(cross_module)

# Focused examples of the current API: return nonzero on a broken contract, including Release builds.
find_package(Threads REQUIRED)
foreach(recipe
    local_wiring sink_output static_addresses static_cancellation
    dynamic_lifetime thread_safe_lifetime dynamic_diagnostics scoped_diagnostics
    link_forwarding static_link_forwarding route_reports)
    add_executable(Sub0Pub_Example_${recipe} ${recipe}.cpp)
    target_link_libraries(Sub0Pub_Example_${recipe} PRIVATE Sub0Pub::Sub0Pub Threads::Threads)
    if(SUB0PUB_BUILD_TESTING AND NOT IS_SUBPROJECT)
        add_test(NAME Sub0Pub_Example_${recipe} COMMAND Sub0Pub_Example_${recipe})
    endif()
endforeach()
