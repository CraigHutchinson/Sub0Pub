# v1 vs v2 comparison benchmarks (not run by ctest; see compare_versions.py, which also builds cmp_sub0pub.cpp
# against the v1.0 header). Targets remain available on demand against the current header.
set(SUB0PUB_COMPARE_INCLUDES
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../bench"
    "${CMAKE_CURRENT_SOURCE_DIR}/../vendor"
    "${CMAKE_CURRENT_SOURCE_DIR}/../collapse")

foreach(name cmp_sub0pub cmp_config cmp_static cmp_mixed)
    add_executable(Sub0Pub_Compare_${name} EXCLUDE_FROM_ALL ${name}.cpp)
    target_link_libraries(Sub0Pub_Compare_${name} PRIVATE Sub0Pub::Sub0Pub)
    target_include_directories(Sub0Pub_Compare_${name} PRIVATE ${SUB0PUB_COMPARE_INCLUDES})
endforeach()
