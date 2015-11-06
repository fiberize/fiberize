cmake_minimum_required(VERSION 3.0)

project(fiberize)

file(GLOB_RECURSE FIBERIZE_SRCS "${CMAKE_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE FIBERIZE_HDRS "${CMAKE_SOURCE_DIR}/include/*.hpp")

find_package(Boost 1.59.0 COMPONENTS context thread system REQUIRED)

add_definitions(-DBOOST_THREAD_VERSION=3)
include_directories("${CMAKE_SOURCE_DIR}/include" ${Boost_INCLUDE_DIRS})
add_library(fiberize ${FIBERIZE_SRCS})
target_link_libraries(fiberize ${Boost_LIBRARIES} pthread)
target_compile_features(fiberize PUBLIC PRIVATE cxx_template_template_parameters cxx_generic_lambdas)

install(TARGETS fiberize RUNTIME DESTINATION bin ARCHIVE DESTINATION lib)
install(FILES ${FIBERIZE_HDRS} DESTINATION include)

add_subdirectory(examples)
add_subdirectory(benchmarks)