# SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
#
# SPDX-License-Identifier: MIT

set(qcoro_dbus_SRCS
    dbus.cpp
)
add_library(QCoroDBus ${qcoro_dbus_SRCS})
add_library(QCoro::DBus ALIAS QCoroDBus)
target_link_libraries(QCoroDBus
    PUBLIC
    QCoro
    Qt${QT_VERSION_MAJOR}::DBus
)
target_include_directories(QCoroDBus
    INTERFACE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
    PUBLIC $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
set_target_properties(QCoroDBus PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS 1)

set(qcoro_dbus_HEADERS
    dbus.h
)

install(
    FILES ${qcoro_dbus_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qcoro/dbus
    COMPONENT Devel
    )

install(TARGETS QCoroDBus EXPORT QCoroTargets)

