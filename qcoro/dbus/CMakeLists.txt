# SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
#
# SPDX-License-Identifier: MIT

add_qcoro_library(
    NAME DBus
    SOURCES
        qcorodbuspendingcall.cpp
    CAMELCASE_HEADERS
        QCoroDBus
        QCoroDBusPendingCall
        QCoroDBusPendingReply
    QCORO_LINK_LIBRARIES
        PUBLIC Coro Core
    QT_LINK_LIBRARIES
        PUBLIC Core DBus
)

