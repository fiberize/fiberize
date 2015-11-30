add_executable(future-test main.cpp)
target_link_libraries(future-test fiberize ${GTEST_BOTH_LIBRARIES})
add_test(NAME future-test COMMAND future-test)
set_tests_properties(future-test PROPERTIES TIMEOUT 15)
