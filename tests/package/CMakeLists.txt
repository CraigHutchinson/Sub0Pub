# Installed-package check: install Sub0Pub into a scratch prefix, then configure, build and run an external project
# that finds it with find_package(Sub0Pub), as a consumer of a release would.
add_test(NAME Sub0Pub_PackageConsumer
    COMMAND ${CMAKE_COMMAND}
        -DSUB0PUB_BINARY_DIR=${PROJECT_BINARY_DIR}
        -DCONSUMER_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/consumer
        -DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/package
        -DCONFIG=$<CONFIG>
        -DGENERATOR=${CMAKE_GENERATOR}
        -DGENERATOR_PLATFORM=${CMAKE_GENERATOR_PLATFORM}
        -DGENERATOR_TOOLSET=${CMAKE_GENERATOR_TOOLSET}
        -DMAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
        -DTOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
        -DCXX_COMPILER=${CMAKE_CXX_COMPILER}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/run_package_test.cmake)
