cmake_minimum_required(VERSION 3.10...3.15)
project(rbhash VERSION 1.0.0 LANGUAGES C CXX)

if(NOT CMAKE_C_STANDARD)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
endif()

if(NOT CMAKE_CXX_STANDARD)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
endif()

option(BUILD_SHARED_LIBS "Build shared library" ON)
option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_BENCHMARKS "Build benchmarks" ON)
option(BUILD_EXAMPLES "Build examples" OFF)

option(DEFINE_HASH_TEST "Define __HASH_TEST__ macro for test" OFF)
option(INSTALL "Install rbhash" ON)

if(DEFINE_HASH_TEST)
add_compile_definitions(__HASH_TEST__)
endif()

# define release version
if(NOT CMAKE_BUILD_TYPE)
set(CMAKE_BUILD_TYPE Release CACHE STRING
    "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
    FORCE)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# pthread and prefer -pthread flag
find_package(Threads REQUIRED)
set(THREADS_PREFER_PTHREAD_FLAG ON)

# if always prefer local Boost, uncomment the following line
# set(Boost_NO_SYSTEM_PATHS ON CACHE BOOL "Do not search system for Boost")

# set BOOST_ROOT or BOOST_LIBRARYDIR environment
# set(BOOST_VERSION 1.65.1)
# find_package(Boost ${BOOST_VERSION} EXACT REQUIRED COMPONENTS
#                 filesystem
#                 locale
#                 program_options
#                 system
#                 thread
#                 regex)

# if(DEFINE_LATENCY_TEST)
#     ADD_DEFINITIONS(-DLATENCY_TEST)
# endif()

set(PUBLIC_INCLUDE_DIR "include/rbhash")

include("${CMAKE_SOURCE_DIR}/cmake/third_party.cmake")

add_library(rbhash INTERFACE)
target_link_libraries(rbhash INTERFACE Threads::Threads rt)
target_include_directories(rbhash INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/include>
)

if(BUILD_TESTS)
enable_testing()
set(INSTALL_GTESTS OFF CACHE BOOl "Don't install gtest" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "Don't install benchmarks" FORCE)

# build google tests (targets: gtest_main, gtest)
add_subdirectory(third_party/googletest)
add_subdirectory(test)
endif()

if(BUILD_BENCHMARKS)
add_subdirectory(benchmark)
endif()

if(BUILD_EXAMPLES)
add_subdirectory(example)
endif()

if(INSTALL)
include(GNUInstallDirs)
install(TARGETS rbhash
    EXPORT rhashTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# include file
install(FILES
    "${PUBLIC_INCLUDE_DIR}/rbhash.h"

    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/rbhash"
)

# binary
# install(FILES
#     # xxx
#     DESTINATION "${CMAKE_INSTALL_BINDIR}/adk"
# )

# doc
# install(FILES
#     # xxx
#     DESTINATION doc
# )
endif()
