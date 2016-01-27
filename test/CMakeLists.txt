cmake_minimum_required(VERSION 2.8)

project(configuru_test)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING
      "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif(NOT CMAKE_BUILD_TYPE)

find_package(Boost REQUIRED filesystem system)
add_definitions(-DBOOST_FILESYSTEM_VERSION=3)
include_directories(${Boost_INCLUDE_DIRS})

MESSAGE(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
MESSAGE(STATUS "CMAKE_CXX_COMPILER_ID: ${CMAKE_CXX_COMPILER_ID}")

add_compile_options(-std=c++11 -Werror -Wall -Wextra)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    MESSAGE(STATUS "Compiling with fsanitize")
    add_compile_options(-fsanitize=memory)
    #add_compile_options(-fsanitize=address)
    #add_compile_options(-fsanitize=undefined)

    add_compile_options(--no-undefined)
    add_compile_options(-Wno-error=unused-command-line-argument)
endif()

file(GLOB source
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../*.cpp"
)

add_executable(configuru_test ${source})

MESSAGE(STATUS "Boost_FILESYSTEM_LIBRARY: ${Boost_FILESYSTEM_LIBRARY}")

find_package(Threads)
target_link_libraries(configuru_test ${Boost_FILESYSTEM_LIBRARY})
target_link_libraries(configuru_test ${Boost_SYSTEM_LIBRARY})
target_link_libraries(configuru_test ${CMAKE_THREAD_LIBS_INIT}) # For pthreads
target_link_libraries(configuru_test dl) # For ldl
