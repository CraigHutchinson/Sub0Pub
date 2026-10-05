# Driver for Sub0Pub_PackageConsumer (see CMakeLists.txt): each step must succeed
function(run)
    execute_process(COMMAND ${ARGN} RESULT_VARIABLE result)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "package test step failed (${result}): ${ARGN}")
    endif()
endfunction()

if(NOT CONFIG)
    set(CONFIG Release)
endif()
file(REMOVE_RECURSE "${WORK_DIR}")

run(${CMAKE_COMMAND} --install "${SUB0PUB_BINARY_DIR}" --prefix "${WORK_DIR}/prefix" --config ${CONFIG})

set(configure ${CMAKE_COMMAND} -S "${CONSUMER_SOURCE_DIR}" -B "${WORK_DIR}/build" -G "${GENERATOR}"
    "-DCMAKE_PREFIX_PATH=${WORK_DIR}/prefix" "-DCMAKE_BUILD_TYPE=${CONFIG}" "-DCMAKE_CXX_COMPILER=${CXX_COMPILER}")
# The consumer builds with the same tools as this project
if(GENERATOR_PLATFORM)
    list(APPEND configure -A "${GENERATOR_PLATFORM}")
endif()
if(GENERATOR_TOOLSET)
    list(APPEND configure -T "${GENERATOR_TOOLSET}")
endif()
if(MAKE_PROGRAM)
    list(APPEND configure "-DCMAKE_MAKE_PROGRAM=${MAKE_PROGRAM}")
endif()
if(TOOLCHAIN_FILE)
    list(APPEND configure "-DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN_FILE}")
endif()
run(${configure})
run(${CMAKE_COMMAND} --build "${WORK_DIR}/build" --config ${CONFIG})
run(${CMAKE_CTEST_COMMAND} --test-dir "${WORK_DIR}/build" -C ${CONFIG} --output-on-failure)
