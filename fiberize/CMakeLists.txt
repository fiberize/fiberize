cmake_minimum_required(VERSION 2.8)

project(fiberize)

file(GLOB_RECURSE FIBERIZE_SRCS "${CMAKE_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE FIBERIZE_HDRS "${CMAKE_SOURCE_DIR}/include/*.hpp")

find_package(Boost 1.59.0 COMPONENTS context thread system REQUIRED)

add_definitions(-DBOOST_THREAD_VERSION=3)

#add_definitions(-DBOOST_USE_SEGMENTED_STACKS)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsplit-stack")

include_directories("${CMAKE_SOURCE_DIR}/include" ${Boost_INCLUDE_DIRS})
add_library(fiberize ${FIBERIZE_SRCS})
target_link_libraries(fiberize ${Boost_LIBRARIES} pthread)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y")

install(TARGETS fiberize RUNTIME DESTINATION bin ARCHIVE DESTINATION lib)
install(FILES ${FIBERIZE_HDRS} DESTINATION include)

add_subdirectory(examples)
add_subdirectory(benchmarks)
