# SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
#
# SPDX-License-Identifier: MIT

add_library(qcoro INTERFACE)
add_library(QCoro::QCoro ALIAS qcoro)
target_include_directories(qcoro
    INTERFACE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
    INTERFACE $<TARGET_PROPERTY:Qt5::Core,INTERFACE_INCLUDE_DIRECTORIES>
    INTERFACE $<TARGET_PROPERTY:Qt5::DBus,INTERFACE_INCLUDE_DIRECTORIES>
    INTERFACE $<TARGET_PROPERTY:Qt5::Network,INTERFACE_INCLUDE_DIRECTORIES>
)

file(GLOB qcoro_HEADERS "*.h")

install(
    FILES ${qcoro_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qcoro
    COMPONENT Devel
)

install(TARGETS qcoro EXPORT QCoroTargets)

