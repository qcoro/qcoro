# SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
#
# SPDX-License-Identifier: MIT

add_qcoro_library(
    NAME Core
    INCLUDEDIR Core
    SOURCES
        qcoroiodevice.cpp
        qcoroprocess.cpp
        qcorotimer.cpp
    CAMELCASE_HEADERS
        QCoroCore
        QCoroIODevice
        QCoroProcess
        QCoroSignal
        QCoroTimer
        QCoroFuture
    QT_LINK_LIBRARIES
        PUBLIC Core
    QCORO_LINK_LIBRARIES
        PUBLIC Coro
)

