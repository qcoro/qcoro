cmake_minimum_required(VERSION 3.14)
project(qcoro)

#-----------------------------------------------------------#
# Compiler Settings
#-----------------------------------------------------------#

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -pedantic")

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcoroutines")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcoroutines-ts -stdlib=libc++")
else()
    message(FATAL_ERROR "Compiler ${CMAKE_CXX_COMPILER_ID} is not currently supported.")
endif()

#-----------------------------------------------------------#
# Dependencies
#-----------------------------------------------------------#

find_package(Threads REQUIRED)
find_package(Qt5Core REQUIRED)
find_package(Qt5DBus REQUIRED)
find_package(Qt5Network REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Concurrent REQUIRED)

#-----------------------------------------------------------#
# Sources
#-----------------------------------------------------------#

add_subdirectory(qcoro)
add_subdirectory(examples)
