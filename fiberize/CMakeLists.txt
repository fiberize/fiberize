cmake_minimum_required(VERSION 3.1)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/Modules/")

################################################################################
### User options.
################################################################################

option(VALGRIND "enable valgrind support" OFF)

if(VALGRIND)
  add_definitions(-DFIBERIZE_VALGRIND -DBOOST_USE_VALGRIND)
endif(VALGRIND)

option(SEGMENTED_STACKS "enable segmented stacks" OFF)

if(SEGMENTED_STACKS)
  add_definitions(-DFIBERIZE_SEGMENTED_STACKS -DBOOST_USE_SEGMENTED_STACKS)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsplit-stack")
endif(SEGMENTED_STACKS)

option(PROFILING "enable profiling" OFF)

if(PROFILING)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg -g")
endif(PROFILING)

option(LTO "link time optimization" ON)

if(LTO)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto")
endif(LTO)

option(TCMALLOC "enable tcmalloc" ON)

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

# libuv
set(LIBUV_USE_STATIC YES)
find_package(LibUV REQUIRED)
include_directories(${LIBUV_INCLUDE_DIRS} BEFORE)

if(NOT HAVE_UV_HRTIME_FAST)
  message(FATAL_ERROR
    "The installed libuv does not support uv_hrtime_fast function. "
    "Please install the libuv version bundled with fiberize.")
endif(NOT HAVE_UV_HRTIME_FAST)

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
target_link_libraries(fiberize ${Boost_LIBRARIES})
target_link_libraries(fiberize Threads::Threads)
target_link_libraries(fiberize ${LIBUV_LIBRARIES})

################################################################################
### tcmalloc
################################################################################

if(TCMALLOC)
  target_link_libraries(fiberize tcmalloc)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
    -fno-builtin-malloc -fno-builtin-calloc\
    -fno-builtin-realloc -fno-builtin-free")
endif(TCMALLOC)

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
