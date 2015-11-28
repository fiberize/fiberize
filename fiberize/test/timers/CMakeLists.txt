add_executable(timers-test main.cpp)
target_link_libraries(timers-test fiberize ${GTEST_BOTH_LIBRARIES})
add_test(NAME timers-test COMMAND timers-test)
set_tests_properties(timers-test PROPERTIES TIMEOUT 15)
