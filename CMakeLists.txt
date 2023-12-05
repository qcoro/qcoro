cmake_minimum_required(VERSION 3.18.4)
set(qcoro_VERSION 0.10.0)
set(qcoro_SOVERSION 0)
project(qcoro LANGUAGES CXX VERSION ${qcoro_VERSION})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(GNUInstallDirs)
include(CTest)
include(FeatureSummary)

#-----------------------------------------------------------#
# Options
#-----------------------------------------------------------#

option(QCORO_BUILD_EXAMPLES "Build examples" ON)
add_feature_info(Examples QCORO_BUILD_EXAMPLES "Build examples")
option(QCORO_ENABLE_ASAN "Build with AddressSanitizer" OFF)
add_feature_info(Asan QCORO_ENABLE_ASAN "Build with AddressSanitizer")
option(QCORO_DISABLE_DEPRECATED_TASK_H "Disable deprecated task.h header" OFF)

if(WIN32 OR APPLE OR ANDROID)
    option(QCORO_WITH_QTDBUS "Build QtDBus support" OFF)
else()
    option(QCORO_WITH_QTDBUS "Build QtDBus support" ON)
endif()
add_feature_info(QtDBus QCORO_WITH_QTDBUS "Build QtDBus support")
option(QCORO_WITH_QTNETWORK "Build QtNetwork support" ON)
add_feature_info(QtNetwork QCORO_WITH_QTNETWORK "Build QtNetwork support")
option(QCORO_WITH_QTWEBSOCKETS "Build QtWebSockets support" ON)
add_feature_info(QtWebSockets QCORO_WITH_QTWEBSOCKETS "Build QtWebSockets support")
option(QCORO_WITH_QTQUICK "Build QtQuick support" ON)
add_feature_info(QtQuick QCORO_WITH_QTQUICK "Build QtQuick support")
option(QCORO_WITH_QML "Build QML integration features" ON)
add_feature_info(QtQml QCORO_WITH_QML "Build QML integration features")
option(QCORO_WITH_QTTEST "Build QtTest support" ON)
add_feature_info(QtTest QCORO_WITH_QTTEST "Build QtTest support")

#-----------------------------------------------------------#
# Dependencies
#-----------------------------------------------------------#

set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

include(cmake/CheckAtomic.cmake)

set(REQUIRED_QT_COMPONENTS Core)
set(REQUIRED_QT5_COMPONENTS)
set(REQUIRED_QT6_COMPONENTS)
if (QCORO_WITH_QTDBUS)
    list(APPEND REQUIRED_QT_COMPONENTS DBus)
endif()
if (QCORO_WITH_QTNETWORK)
    list(APPEND REQUIRED_QT_COMPONENTS Network)
endif()
if (QCORO_WITH_QTWEBSOCKETS)
    list(APPEND REQUIRED_QT_COMPONENTS WebSockets)
endif()
if (QCORO_WITH_QTQUICK)
    list(APPEND REQUIRED_QT_COMPONENTS Quick QuickPrivate)
endif()
if (QCORO_WITH_QML)
    list(APPEND REQUIRED_QT_COMPONENTS Qml)
    # Qt6 needs access to private API
    list(APPEND REQUIRED_QT6_COMPONENTS QmlPrivate)
endif()
if (QCORO_WITH_QTTEST)
    list(APPEND REQUIRED_QT_COMPONENTS Test)
endif()
if (QCORO_BUILD_EXAMPLES)
    list(APPEND REQUIRED_QT_COMPONENTS Widgets Concurrent)
endif()
if (BUILD_TESTING)
    list(APPEND REQUIRED_QT_COMPONENTS Test Concurrent)
endif()

set(MIN_REQUIRED_QT5_VERSION "5.12")
set(MIN_REQUIRED_QT6_VERSION "6.2.0")

include(cmake/QCoroFindQt.cmake)
# Find Qt. If USE_QT_VERSION is not set, it will try to look for Qt6 first
# and fallback to Qt5 otherwise.
qcoro_find_qt(
    QT_VERSION "${USE_QT_VERSION}"
    COMPONENTS "${REQUIRED_QT_COMPONENTS}"
    QT5_COMPONENTS "${REQUIRED_QT5_COMPONENTS}"
    QT6_COMPONENTS "${REQUIRED_QT6_COMPONENTS}"
    FOUND_VER_VAR QT_VERSION_MAJOR
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/config.h
)

#-----------------------------------------------------------#
# Compiler Settings
#-----------------------------------------------------------#

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)

if (MSVC)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /W4 /WX")
    # Disable warning C5054: "operator '&': deprecated between enumerations of different types" caused by QtWidgets/qsizepolicy.h
    # Disable warning C4127: "conditional expression is constant" caused by QtCore/qiterable.h
    if ("${QT_VERSION_MAJOR}" STREQUAL "6" AND "${Qt6_VERSION}" VERSION_GREATER_EQUAL "6.4.0" AND "${Qt6_VERSION}" VERSION_LESS "6.5.3")
        # Disable warning C4702: "unreachable code" caused by QtTest/qtestcase.h - fixed in Qt 6.5.3
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /wd5054 /wd4127 /wd4702")
    endif()

    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        # Explicitly enable exceptions support for clang-cl (it's only enabled by CMake when targeting the Windows-MSVC platform,
        # see https://github.com/danvratil/qcoro/issues/90 for details)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")
    endif()
else()
    # Only enable strict warnings in debug mode
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra -Werror -pedantic")
endif()

if (QCORO_ENABLE_ASAN)
    if (MSVC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /INCREMENTAL:NO")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /INCREMENTAL:NO")
        if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
        else()
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fsanitize=address")
        endif()
    else()
        if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS_EQUAL "11.0.0")
            # I tried my best, but there are crashes when combining asan and coroutines.
            message(WARNING "ASAN not supported for Clang<=11.0")
        else()
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer -fsanitize-recover=address")
        endif()
    endif()
endif()

add_compile_definitions(
    QT_NO_CAST_FROM_ASCII
    QT_NO_CAST_TO_ASCII
    QT_NO_URL_CAST_FROM_STRING
    QT_NO_CAST_FROM_BYTEARRAY
    QT_USE_STRINGBUILDER
    QT_NO_NARROWING_CONVERSIONS_IN_CONNECT
    QT_NO_KEYWORDS
    QT_NO_FOREACH
)
if (NOT WIN32)
    # strict iterators on MSVC only work when Qt itself is also built with them,
    # which is not usually the case. Otherwise there are linking issues.
    add_compile_definitions(QT_STRICT_ITERATORS)
endif()

include(qcoro/QCoroMacros.cmake)
qcoro_enable_coroutines()

include(cmake/CodeCoverage.cmake)
add_code_coverage()
add_code_coverage_all_targets(EXCLUDE "${CMAKE_BINARY_DIR}" tests/utils/*)

#-----------------------------------------------------------#
# Definitions
#-----------------------------------------------------------#

# debug suffixes for qmake compatibility
if(WIN32)
    set(CMAKE_DEBUG_POSTFIX "d")
elseif(APPLE)
    set(CMAKE_DEBUG_POSTFIX "_debug")
else()
    set(CMAKE_DEBUG_POSTFIX "")
endif()

set(QCORO_TARGET_PREFIX "QCoro${QT_VERSION_MAJOR}")
set(QCORO_INSTALL_INCLUDEDIR "${CMAKE_INSTALL_INCLUDEDIR}/qcoro${QT_VERSION_MAJOR}")

#-----------------------------------------------------------#
# Sources
#-----------------------------------------------------------#

set(QCORO_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
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

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/config.h
    DESTINATION ${QCORO_INSTALL_INCLUDEDIR}/qcoro
    COMPONENT Devel
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/QCoroConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/QCoro${QT_VERSION_MAJOR}Config.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/QCoro${QT_VERSION_MAJOR}"
    PATH_VARS CMAKE_INSTALL_INCLUDEDIR
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/QCoro${QT_VERSION_MAJOR}ConfigVersion.cmake"
    VERSION ${qcoro_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/QCoro${QT_VERSION_MAJOR}Config.cmake"
          "${CMAKE_CURRENT_BINARY_DIR}/QCoro${QT_VERSION_MAJOR}ConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/QCoro${QT_VERSION_MAJOR}"
    COMPONENT Devel
)

#-----------------------------------------------------------#
# Summary
#-----------------------------------------------------------#

feature_summary(FATAL_ON_MISSING_REQUIRED_PACKAGES WHAT ALL)
