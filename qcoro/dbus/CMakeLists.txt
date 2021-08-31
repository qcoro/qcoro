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
    INTERFACE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>;$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/qcoro/dbus>
    PUBLIC $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>;$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/qcoro>
    PUBLIC $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/QCoro>
)
set_target_properties(QCoroDBus PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS 1)

generate_headers(
    qcoro_dbus_CamelCase_HEADERS
    HEADER_NAMES
        QCoroDBus
        DBus
    OUTPUT_DIR QCoro/DBus
    ORIGINAL_HEADERS_VAR qcoro_dbus_HEADERS
)

install(
    FILES ${qcoro_dbus_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qcoro
    COMPONENT Devel
)
install(
    FILES ${qcoro_dbus_CamelCase_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/QCoro
    COMPONENT Devel
)

install(TARGETS QCoroDBus EXPORT QCoroTargets)

