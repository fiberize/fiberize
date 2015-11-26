add_executable(kill-test main.cpp)
target_link_libraries(kill-test fiberize ${GTEST_BOTH_LIBRARIES})
add_test(NAME kill-test COMMAND kill-test)
set_tests_properties(kill-test PROPERTIES TIMEOUT 15)
