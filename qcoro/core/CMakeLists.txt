# SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
#
# SPDX-License-Identifier: MIT

add_qcoro_library(
    NAME Core
    INCLUDEDIR Core
    SOURCES
        qcoroiodevice.cpp
        qcoroiodevice_p.cpp
        qcoroprocess.cpp
        qcorothread.cpp
        qcorotimer.cpp
    CAMELCASE_HEADERS
        QCoroCore
        QCoroIODevice
        QCoroProcess
        QCoroSignal
        QCoroThread
        QCoroTimer
        QCoroFuture
    HEADERS
        impl/isqprivatesignal.h
    QT_LINK_LIBRARIES
        PUBLIC Core
    QCORO_LINK_LIBRARIES
        PUBLIC Coro
)
