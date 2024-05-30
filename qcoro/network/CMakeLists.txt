# SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
#
# SPDX-License-Identifier: MIT

add_qcoro_library(
    NAME Network
    SOURCES
        qcoroabstractsocket.cpp
        qcorolocalsocket.cpp
        qcoronetworkreply.cpp
        qcorosocketnotifier.cpp
        qcorotcpserver.cpp
    CAMELCASE_HEADERS
        QCoroNetwork
        QCoroAbstractSocket
        QCoroLocalSocket
        QCoroNetworkReply
        QCoroSocketNotifier
        QCoroTcpServer
    QCORO_LINK_LIBRARIES
        PUBLIC Coro Core
    QT_LINK_LIBRARIES
        PUBLIC Core Network
)

