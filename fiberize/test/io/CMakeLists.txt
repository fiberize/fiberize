add_executable(io-test main.cpp)
target_link_libraries(io-test fiberize ${GTEST_LIBRARY})
add_test(NAME io-test COMMAND io-test)
set_tests_properties(io-test PROPERTIES TIMEOUT 15)
