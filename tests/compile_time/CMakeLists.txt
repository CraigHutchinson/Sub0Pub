# Timing is opt-in; only quick harness contract tests belong in ordinary CTest runs.
find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_Interpreter_FOUND)
    add_test(NAME Sub0Pub_CompileTimeContracts
             COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/test_compare.py")
endif()
