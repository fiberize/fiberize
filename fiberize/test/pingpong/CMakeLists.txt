add_executable(pingpong-test main.cpp)
target_link_libraries(pingpong-test fiberize ${GTEST_BOTH_LIBRARIES})
add_test(NAME pingpong-test COMMAND pingpong-test)
set_tests_properties(pingpong-test PROPERTIES TIMEOUT 15)
