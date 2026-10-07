# The audit build (SUB0PUB_AUDIT): one ledger per message type, fed by the runtime broker and by StaticTo / StaticFirst
# lists, with its findings available to a test and reported at exit

find_package(Threads REQUIRED)
add_executable(Sub0Pub_AuditTests test_audit.cpp)
target_link_libraries(Sub0Pub_AuditTests PRIVATE Sub0Pub_TestMain Sub0Pub::Sub0Pub Threads::Threads)
target_include_directories(Sub0Pub_AuditTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../vendor")
add_test(NAME Sub0Pub_AuditTests COMMAND Sub0Pub_AuditTests)

# The report at exit: passes only if SUB0PUB_AUDIT_EXIT runs after it and sees the program's one finding
add_executable(Sub0Pub_AuditExitReport exit_report.cpp)
target_link_libraries(Sub0Pub_AuditExitReport PRIVATE Sub0Pub::Sub0Pub Threads::Threads)
add_test(NAME Sub0Pub_AuditExitReport COMMAND Sub0Pub_AuditExitReport)
# ...and what it wrote (a test with a pass expression ignores the exit status, so this is a second test)
add_test(NAME Sub0Pub_AuditExitReportText COMMAND Sub0Pub_AuditExitReport)
set_tests_properties(Sub0Pub_AuditExitReportText PROPERTIES
    PASS_REGULAR_EXPRESSION "sub0pub audit: 1 finding[^s].*Alarm: 1 of 1 publication reached no receiver; 1 by .*Sensor")

# A build without the audit: both functions are constants
add_library(Sub0Pub_AuditOff OBJECT audit_off.cpp)
target_link_libraries(Sub0Pub_AuditOff PRIVATE Sub0Pub::Sub0Pub)
