# SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
#
# SPDX-License-Identifier: MIT

if (QCORO_QT_HAS_COMPAT_ABI)
    set(QCOROFUTURE_HEADER "QCoroFuture")
endif()

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
        ${QCOROFUTURE_HEADER}
    QT_LINK_LIBRARIES
        PUBLIC Core
)

