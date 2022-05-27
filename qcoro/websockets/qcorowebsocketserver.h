// SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "qcorotask.h"
#include "qcorowebsockets_export.h"

#include <chrono>

class QWebSocket;
class QWebSocketServer;

namespace QCoro::detail {

class QCOROWEBSOCKETS_EXPORT QCoroWebSocketServer {
public:
    explicit QCoroWebSocketServer(QWebSocketServer *server);

    Task<QWebSocket *> nextPendingConnection(std::chrono::milliseconds timeout = std::chrono::milliseconds{-1});

private:
    QWebSocketServer *mServer;
};



} // namespace QCoro::detail

auto inline qCoro(QWebSocketServer *server) noexcept {
    return QCoro::detail::QCoroWebSocketServer{server};
}

auto inline qCoro(QWebSocketServer &server) noexcept {
    return QCoro::detail::QCoroWebSocketServer{&server};
}
