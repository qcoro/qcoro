# SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
#
# SPDX-License-Identifier: MIT

set(qcoro_core_SRCS
    iodevice.cpp
    qcoroiodevice.cpp
    qcoroprocess.cpp
)
add_library(QCoroCore ${qcoro_core_SRCS})
add_library(QCoro::Core ALIAS QCoroCore)
target_link_libraries(QCoroCore
    PUBLIC
    QCoro
    Qt${QT_VERSION_MAJOR}::Core
)
target_include_directories(QCoroCore
    INTERFACE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
    PUBLIC $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
set_target_properties(QCoroCore PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS 1)

set(qcoro_core_HEADERS
    iodevice.h
    qcoroiodevice.h
    qcoroprocess.h
    qcorosignal.h
    timer.h
)

if (QT_HAS_COMPAT_ABI)
    set(qcoro_core_HEADERS ${qcoro_core_HEADERS} future.h)
endif()

install(
    FILES ${qcoro_core_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qcoro/core
    COMPONENT Devel
)

install(TARGETS QCoroCore EXPORT QCoroTargets)

