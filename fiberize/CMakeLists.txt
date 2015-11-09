cmake_minimum_required(VERSION 2.8)

# User options.
option(VALGRIND "enable valgrind support" ON)

if(VALGRIND)
  add_definitions(-DFIBERIZE_VALGRIND -DBOOST_USE_VALGRIND)
endif(VALGRIND)

option(SEGMENTED_STACKS "enable segmented stacks [work in progress]" OFF)

if(SEGMENTED_STACKS)
  add_definitions(-DFIBERIZE_SEGMENTED_STACKS -DBOOST_USE_SEGMENTED_STACKS)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsplit-stack")
endif(SEGMENTED_STACKS)

# Project files.
project(fiberize)

file(GLOB_RECURSE FIBERIZE_SRCS "${CMAKE_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE FIBERIZE_HDRS "${CMAKE_SOURCE_DIR}/include/*.hpp")

# Dependencies.
find_package(Boost 1.59.0 COMPONENTS context thread system REQUIRED)
add_definitions(-DBOOST_THREAD_VERSION=3)

# Enable C++1y and warnings
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y -Wall -Wextra")

# Make the include directory visible in all projects.
include_directories("${CMAKE_SOURCE_DIR}/include" ${Boost_INCLUDE_DIRS})

# Add the library.
add_library(fiberize ${FIBERIZE_SRCS})
target_link_libraries(fiberize ${Boost_LIBRARIES} pthread)

# Install the library.
install(TARGETS fiberize RUNTIME DESTINATION bin ARCHIVE DESTINATION lib)
install(FILES ${FIBERIZE_HDRS} DESTINATION include)

add_subdirectory(examples)
add_subdirectory(benchmarks)
