// SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "qcorowebsocketserver.h"
#include "qcorosignal.h"

#include <QWebSocket>
#include <QWebSocketServer>

using namespace QCoro::detail;

QCoroWebSocketServer::QCoroWebSocketServer(QWebSocketServer *server)
    : mServer(server)
{}

QCoro::Task<QWebSocket *> QCoroWebSocketServer::nextPendingConnection(std::chrono::milliseconds timeout)
{
    auto * const server = mServer;
    if (!server->isListening()) {
        co_return nullptr;
    }

    if (server->hasPendingConnections()) {
        co_return server->nextPendingConnection();
    }

    const auto result = co_await qCoro(server, &QWebSocketServer::newConnection, timeout);
    if (!result.has_value()) {
        co_return nullptr;
    }

    co_return server->nextPendingConnection();
}
