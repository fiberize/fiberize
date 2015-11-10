add_executable(fibonacci main.cpp)
target_link_libraries(fibonacci fiberize ${GTEST_BOTH_LIBRARIES})
add_test(NAME fibonacci COMMAND fibonacci)
set_tests_properties(fibonacci PROPERTIES TIMEOUT 15)