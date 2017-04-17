cmake_minimum_required(VERSION 2.8)

option(CONFIGURU_VALUE_SEMANTICS "CONFIGURU_VALUE_SEMANTICS" ON)
if (CONFIGURU_VALUE_SEMANTICS)
    add_compile_options(-DCONFIGURU_VALUE_SEMANTICS=1)
else()
    add_compile_options(-DCONFIGURU_VALUE_SEMANTICS=0)
endif(CONFIGURU_VALUE_SEMANTICS)

option(CONFIGURU_IMPLICIT_CONVERSIONS "CONFIGURU_IMPLICIT_CONVERSIONS" ON)
if (CONFIGURU_IMPLICIT_CONVERSIONS)
    add_compile_options(-DCONFIGURU_IMPLICIT_CONVERSIONS=1)
else()
    add_compile_options(-DCONFIGURU_IMPLICIT_CONVERSIONS=0)
endif(CONFIGURU_IMPLICIT_CONVERSIONS)

project(configuru_test)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING
      "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif(NOT CMAKE_BUILD_TYPE)

find_package(Boost REQUIRED filesystem system)
add_definitions(-DBOOST_FILESYSTEM_VERSION=3)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
include_directories(SYSTEM .)

MESSAGE(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
MESSAGE(STATUS "CMAKE_CXX_COMPILER_ID: ${CMAKE_CXX_COMPILER_ID}")

add_compile_options(-std=c++14 -Werror -Wall -Wextra)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    add_compile_options(
        -Weverything
        -Wno-c++98-compat
        -Wno-c++98-compat-pedantic
        -Wno-exit-time-destructors             # configuru_test.cpp
        -Wno-float-equal                       # configuru_test.cpp
        -Wno-global-constructors               # configuru_test.cpp
        -Wno-gnu-zero-variadic-macro-arguments # configuru_test.cpp vs Loguru
        -Wno-missing-prototypes                # configuru_test.cpp
        -Wno-old-style-cast                    # configuru_test.cpp
        -Wno-padded
    )
endif() # Clang

file(GLOB source
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../*.cpp"
)

add_executable(configuru_test ${source})

find_package(Threads)
target_link_libraries(configuru_test ${Boost_FILESYSTEM_LIBRARY})
target_link_libraries(configuru_test ${Boost_SYSTEM_LIBRARY})
target_link_libraries(configuru_test ${CMAKE_THREAD_LIBS_INIT}) # For pthreads
target_link_libraries(configuru_test dl) # For ldl
