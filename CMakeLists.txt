cmake_minimum_required(VERSION 3.19)
set(qcoro_VERSION 0.1)
project(qcoro LANGUAGES CXX VERSION ${qcoro_VERSION})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(GNUInstallDirs)
include(CTest)
include(FeatureSummary)
include(HasCompatibleStlAbi)

#-----------------------------------------------------------#
# Options
#-----------------------------------------------------------#

option(QCORO_BUILD_EXAMPLES "Build examples" ON)
add_feature_info(Examples QCORO_BUILD_EXAMPLES "Build examples")
option(QCORO_ENABLE_ASAN "Build with AddressSanitizer" OFF)
add_feature_info(Asan QCORO_ENABLE_ASAN "Build with AddressSanitizer")

#-----------------------------------------------------------#
# Compiler Settings
#-----------------------------------------------------------#

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /WX")
    # Disable warning C5054: "operator '&': deprecated between enumerations of different types" caused by QtWidgets/qsizepolicy.h
    # Disable warning C4127: "conditional expression is constant" caused by QtCore/qiterable.h
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd5054 /wd4127")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -pedantic")
endif()

if (QCORO_ENABLE_ASAN)
    if (MSVC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fsanitize=address")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
    endif()
endif()

include(QCoroMacros.cmake)
qcoro_enable_coroutines()

#-----------------------------------------------------------#
# Dependencies
#-----------------------------------------------------------#

find_package(Threads REQUIRED)

set(REQUIRED_QT_COMPONENTS Core DBus Network Widgets Concurrent)
if (BUILD_TESTING)
    list(APPEND REQUIRED_QT_COMPONENTS Test)
endif()

if (NOT USE_QT_VERSION)
    # FIXME: find_package(QT NAMES Qt6 Qt5 ...) seems to prefer Qt5 on my system
    find_package(Qt6 COMPONENTS ${REQUIRED_QT_COMPONENTS})
    if (NOT Qt6_FOUND)
        find_package(Qt5 COMPONENTS ${REQUIRED_QT_COMPONENTS} REQUIRED)
        find_package(Qt5Core COMPONENTS Private REQUIRED)
        set(QT_VERSION_MAJOR 5)
    else()
        find_package(Qt6Core COMPONENTS Private REQUIRED)
        set(QT_VERSION_MAJOR 6)
    endif()
else()
    find_package(Qt${USE_QT_VERSION} COMPONENTS ${REQUIRED_QT_COMPONENTS} REQUIRED)
    find_package(Qt${USE_QT_VERSION}Core COMPONENTS Private)
    set(QT_VERSION_MAJOR ${USE_QT_VERSION})
endif()

has_compatible_stl_abi(QT_HAS_COMPAT_ABI)
if (NOT QT_HAS_COMPAT_ABI)
    message(WARNING "Qt is built with an incompatible STL, some features will be disabled!")
endif()

#-----------------------------------------------------------#
# Sources
#-----------------------------------------------------------#

add_subdirectory(qcoro)
if (QCORO_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
if (BUILD_TESTING)
    add_subdirectory(tests)
endif()

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
              "${CMAKE_CURRENT_SOURCE_DIR}/QCoroMacros.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/QCoro
        COMPONENT Devel)

install(EXPORT QCoroTargets
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/QCoro"
        FILE QCoroTargets.cmake
        NAMESPACE QCoro::)

#-----------------------------------------------------------#
# Summary
#-----------------------------------------------------------#

feature_summary(FATAL_ON_MISSING_REQUIRED_PACKAGES WHAT ALL)


