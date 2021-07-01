# SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
#
# SPDX-License-Identifier: MIT

set(qcoro_SRCS
    dbus.cpp
    iodevice.cpp
    network.cpp
    qcoroabstractsocket.cpp
    qcoroiodevice.cpp
    qcorolocalsocket.cpp
    qcoronetworkreply.cpp
    qcoroprocess.cpp
    qcorotcpserver.cpp
    task.cpp
)
add_library(QCoro SHARED ${qcoro_SRCS})
add_library(QCoro::QCoro ALIAS QCoro)
target_link_libraries(QCoro
    PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::CorePrivate
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::DBus
)
target_include_directories(QCoro
    INTERFACE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
    PUBLIC $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
set_target_properties(QCoro PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS 1)

set(qcoro_HEADERS
    concepts.h
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
    qcorotcpserver.h
    task.h
    timer.h
)

set(qcoro_IMPL_HEADERS
    impl/waitoperationbase.h
)

if (QT_HAS_COMPAT_ABI)
    set(qcoro_HEADERS ${qcoro_HEADERS} future.h)
endif()

install(
    FILES ${qcoro_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qcoro
    COMPONENT Devel
)
install(
    FILES ${qcoro_IMPL_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qcoro/impl
    COMPONENT Devel
)

install(TARGETS QCoro EXPORT QCoroTargets)

