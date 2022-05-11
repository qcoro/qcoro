# SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>
#
# SPDX-License-Identifier: MIT

add_qcoro_library(
    NAME WebSockets
    SOURCES
        qcorowebsocket.cpp
        qcorowebsocketserver.cpp
    CAMELCASE_HEADERS
        QCoroWebSockets
        QCoroWebSocket
        QCoroWebSocketServer
    QCORO_LINK_LIBRARIES
        PUBLIC Coro Core
    QT_LINK_LIBRARIES
        PUBLIC Core Network WebSockets
)
