cmake_minimum_required(VERSION 2.8)

project(formater)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING
      "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif(NOT CMAKE_BUILD_TYPE)

find_package(Boost REQUIRED filesystem system)
add_definitions(-DBOOST_FILESYSTEM_VERSION=3)
include_directories(${Boost_INCLUDE_DIRS})

MESSAGE(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

add_compile_options(-std=c++11 -Werror -Wall -Wextra)

file(GLOB source
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../*.cpp"
)

add_executable(formater ${source})

find_package(Threads)
target_link_libraries(formater ${Boost_FILESYSTEM_LIBRARY})
target_link_libraries(formater ${Boost_SYSTEM_LIBRARY})
target_link_libraries(formater ${CMAKE_THREAD_LIBS_INIT}) # For pthreads
target_link_libraries(formater dl) # For ldl
