# SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
#
# SPDX-License-Identifier: MIT

add_library(qcoro)
target_include_directories(qcoro
    INTERFACE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
)
target_link_libraries(qcoro Qt5::Core Qt5::DBus Qt5::Network)

