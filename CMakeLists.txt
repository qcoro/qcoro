cmake_minimum_required(VERSION 3.14)
set(qcoro_VERSION 0.1)
project(qcoro LANGUAGES CXX VERSION ${qcoro_VERSION})

include(GNUInstallDirs)

#-----------------------------------------------------------#
# Compiler Settings
#-----------------------------------------------------------#

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -pedantic -fsanitize=address -fno-omit-frame-pointer")

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

set(THREADS_PREFER_PTHREAD_FLAG ON)
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

#-----------------------------------------------------------#
# Installation
#-----------------------------------------------------------#

include(CMakePackageConfigHelpers)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/QCoroConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/QCoroConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/QCoro
    PATH_VARS CMAKE_INSTALL_INCLUDEDIR)

write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/QCoroConfigVersion.cmake"
        VERSION ${qcoro_VERSION}
        COMPATIBILITY SameMajorVersion)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/QCoroConfig.cmake"
              "${CMAKE_CURRENT_BINARY_DIR}/QCoroConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/QCoro
        COMPONENT Devel)

install(EXPORT QCoroTargets
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/QCoro"
        FILE QCoroTargets.cmake
        NAMESPACE QCoro::)

