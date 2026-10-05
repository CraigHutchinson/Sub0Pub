# Header hygiene for the split library (include/sub0pub/): each header compiles on its own, and each area's entry
# header pulls in no other area (static wiring and IPC need no runtime broker; the broker needs no wiring or IPC).

# One translation unit per header, generated at configure time: a header missing one of its includes fails the build
file(GLOB_RECURSE SUB0PUB_HEADERS RELATIVE "${PROJECT_SOURCE_DIR}/include" CONFIGURE_DEPENDS
     "${PROJECT_SOURCE_DIR}/include/sub0pub/*.hpp")
set(SUB0PUB_HEADER_TUS)
foreach(header ${SUB0PUB_HEADERS})
    string(MAKE_C_IDENTIFIER "${header}" id)
    set(tu "${CMAKE_CURRENT_BINARY_DIR}/self_contained/${id}.cpp")
    file(CONFIGURE OUTPUT "${tu}" CONTENT "#include \"${header}\" // compiles on its own\n")
    list(APPEND SUB0PUB_HEADER_TUS "${tu}")
endforeach()
add_library(Sub0Pub_HeadersSelfContained OBJECT ${SUB0PUB_HEADER_TUS})
target_link_libraries(Sub0Pub_HeadersSelfContained PRIVATE Sub0Pub::Sub0Pub)

# Each entry header alone, with #error checks on the include guards of the areas it must not reach
add_executable(Sub0Pub_HeaderIsolation main.cpp wiring_only.cpp ipc_only.cpp broker_only.cpp config_only.cpp)
target_link_libraries(Sub0Pub_HeaderIsolation PRIVATE Sub0Pub::Sub0Pub)
add_test(NAME Sub0Pub_HeaderIsolation COMMAND Sub0Pub_HeaderIsolation)

# Direct header consumers cannot bypass the baseline by omitting the exported CMake target.
foreach(standard 17 20)
    set(target Sub0Pub_RejectCxx${standard})
    add_library(${target} OBJECT EXCLUDE_FROM_ALL config_only.cpp)
    target_include_directories(${target} PRIVATE "${PROJECT_SOURCE_DIR}/include")
    set_target_properties(${target} PROPERTIES CXX_STANDARD ${standard} CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
    add_test(NAME ${target}
             COMMAND ${CMAKE_COMMAND} --build "${CMAKE_BINARY_DIR}" --target ${target} --config $<CONFIG>)
    set_tests_properties(${target} PROPERTIES PASS_REGULAR_EXPRESSION "Sub0Pub v2 requires C\\+\\+23"
                         RESOURCE_LOCK cmake_build)
endforeach()
