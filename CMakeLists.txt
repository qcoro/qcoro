cmake_minimum_required(VERSION 3.14)
project(qoro)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_AUTOMOC ON)

find_package(Threads REQUIRED)
find_package(Qt5Core REQUIRED)
find_package(Qt5DBus REQUIRED)

add_subdirectory(examples)
