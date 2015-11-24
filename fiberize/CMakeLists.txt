cmake_minimum_required(VERSION 3.1)

################################################################################
### User options.
################################################################################

option(VALGRIND "enable valgrind support" OFF)

if(VALGRIND)
  add_definitions(-DFIBERIZE_VALGRIND -DBOOST_USE_VALGRIND)
endif(VALGRIND)

option(SEGMENTED_STACKS "enable segmented stacks [work in progress]" OFF)

if(SEGMENTED_STACKS)
  add_definitions(-DFIBERIZE_SEGMENTED_STACKS -DBOOST_USE_SEGMENTED_STACKS)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsplit-stack")
endif(SEGMENTED_STACKS)

option(PROFILING "enable profiling" OFF)

if(PROFILING)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
endif(PROFILING)

################################################################################
### Project files.
################################################################################

project(fiberize)

file(GLOB_RECURSE FIBERIZE_SRCS "${CMAKE_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE FIBERIZE_HDRS "${CMAKE_SOURCE_DIR}/include/*.hpp")

################################################################################
### Testing
################################################################################

enable_testing()

################################################################################
### Dependencies.
################################################################################

# Boost
find_package(Boost 1.51.0 COMPONENTS context thread system REQUIRED)
add_definitions(-DBOOST_THREAD_VERSION=3)
include_directories(${Boost_INCLUDE_DIRS})

# pthread
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Google test
find_package(GTest 1.0 REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

################################################################################
### Enable C++1y and warnings
################################################################################
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y -Wall -Wextra")

################################################################################
### Make the include directory visible in all projects.
################################################################################
include_directories("${CMAKE_SOURCE_DIR}/include")

################################################################################
### fiberize library
################################################################################

add_library(fiberize ${FIBERIZE_SRCS})
target_link_libraries(fiberize ${Boost_LIBRARIES} Threads::Threads)

################################################################################
### tcmalloc
################################################################################

target_link_libraries(fiberize tcmalloc)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
      -fno-builtin-malloc -fno-builtin-calloc\
      -fno-builtin-realloc -fno-builtin-free")

################################################################################
### Installation
################################################################################

include(InstallRequiredSystemLibraries)

install(TARGETS fiberize RUNTIME DESTINATION bin ARCHIVE DESTINATION lib)
install(FILES ${FIBERIZE_HDRS} DESTINATION include)

################################################################################
### Examples and benchmarks
################################################################################
add_subdirectory(examples)
add_subdirectory(benchmarks)
add_subdirectory(test)

################################################################################
### Packaging
################################################################################

set(CPACK_GENERATOR "TGZ;ZIP;DEB;RPM")
set(CPACK_PACKAGE_NAME "fiberize")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
  "A framework for high performance parallel and distributed systems")
set(CPACK_PACKAGE_VENDOR "fiberize")
set(CPACK_PACKAGE_CONTACT "pawel@livetalk.lol")
set(CPACK_PACKAGE_VERSION_MAJOR "0")
set(CPACK_PACKAGE_VERSION_MINOR "1")
set(CPACK_PACKAGE_VERSION_PATCH "0")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "CMake ${CMake_VERSION_MAJOR}.${CMake_VERSION_MINOR}")
set(CPACK_STRIP_FILES "lib/libfiberize.a")
set(CPACK_SOURCE_STRIP_FILES "")

include(CPack)
