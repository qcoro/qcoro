# SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
#
# SPDX-License-Identifier: MIT

add_library(qcoro)
add_library(QCoro::QCoro ALIAS qcoro)
target_include_directories(qcoro
    INTERFACE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
)
target_link_libraries(qcoro Qt5::Core Qt5::DBus Qt5::Network)

file(GLOB qcoro_HEADERS "*.h")

install(
    FILES ${qcoro_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qcoro
    COMPONENT Devel
)

install(TARGETS qcoro EXPORT QCoroTargets)

