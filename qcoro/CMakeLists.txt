# SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
#
# SPDX-License-Identifier: MIT

add_library(qcoro INTERFACE)
add_library(QCoro::QCoro ALIAS qcoro)
target_include_directories(qcoro
    INTERFACE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
    INTERFACE $<TARGET_PROPERTY:Qt${QT_VERSION_MAJOR}::Core,INTERFACE_INCLUDE_DIRECTORIES>
    INTERFACE $<TARGET_PROPERTY:Qt${QT_VERSION_MAJOR}::DBus,INTERFACE_INCLUDE_DIRECTORIES>
    INTERFACE $<TARGET_PROPERTY:Qt${QT_VERSION_MAJOR}::Network,INTERFACE_INCLUDE_DIRECTORIES>
)

set(qcoro_HEADERS
    coro.h
    coroutine.h
    dbus.h
    iodevice.h
    macros.h
    network.h
    qcoroabstractsocket.h
    qcoroiodevice.h
    qcorolocalsocket.h
    qcoronetworkreply.h
    qcoroprocess.h
    qcorosignal.h
    task.h
    timer.h
)

if (QT_HAS_COMPAT_ABI)
    set(qcoro_HEADERS ${qcoro_HEADERS} future.h)
endif()

install(
    FILES ${qcoro_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qcoro
    COMPONENT Devel
)

install(TARGETS qcoro EXPORT QCoroTargets)

