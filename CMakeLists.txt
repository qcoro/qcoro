cmake_minimum_required(VERSION 3.19)
set(qcoro_VERSION 0.5.0)
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

if(WIN32 OR APPLE OR ANDROID)
    option(QCORO_WITH_QTDBUS "Build QtDBus support" OFF)
else()
    option(QCORO_WITH_QTDBUS "Build QtDBus support" ON)
endif()
add_feature_info(QtDBus QCORO_WITH_QTDBUS "Build QtDBus support")
option(QCORO_WITH_QTNETWORK "Build QtNetwork support" ON)
add_feature_info(QtNetwork QCORO_WITH_QTNETWORK "Build QtNetwork support")

#-----------------------------------------------------------#
# Dependencies
#-----------------------------------------------------------#

set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

include(cmake/CheckAtomic.cmake)

set(REQUIRED_QT_COMPONENTS Core)
if (QCORO_WITH_QTDBUS)
    list(APPEND REQUIRED_QT_COMPONENTS DBus)
endif()
if (QCORO_WITH_QTNETWORK)
    list(APPEND REQUIRED_QT_COMPONENTS Network)
endif()
if (QCORO_BUILD_EXAMPLES)
    list(APPEND REQUIRED_QT_COMPONENTS Widgets Concurrent)
endif()
if (BUILD_TESTING)
    list(APPEND REQUIRED_QT_COMPONENTS Test Concurrent)
endif()

set(MIN_REQUIRED_QT5_VERSION "5.12")
set(MIN_REQUIRED_QT6_VERSION "6.2.0")


if (NOT USE_QT_VERSION)
    # FIXME: find_package(QT NAMES Qt6 Qt5 ...) seems to prefer Qt5 on my system
    find_package(Qt6 ${MIN_REQUIRED_QT6_VERSION} COMPONENTS ${REQUIRED_QT_COMPONENTS})
    if (NOT Qt6_FOUND)
        find_package(Qt5 ${MIN_REQUIRED_QT5_VERSION} COMPONENTS ${REQUIRED_QT_COMPONENTS} REQUIRED)
        set(QT_VERSION_MAJOR 5)
    else()
        set(QT_VERSION_MAJOR 6)
    endif()
else()
    find_package(Qt${USE_QT_VERSION} ${MIN_REQUIRED_QT${USE_QT_VERSION}_VERSION}
                 COMPONENTS ${REQUIRED_QT_COMPONENTS} REQUIRED)
    set(QT_VERSION_MAJOR ${USE_QT_VERSION})
endif()

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

include(qcoro/QCoroMacros.cmake)
qcoro_enable_coroutines()

#-----------------------------------------------------------#
# Definitions
#-----------------------------------------------------------#

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


