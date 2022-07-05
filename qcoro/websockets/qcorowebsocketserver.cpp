// SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "qcorowebsocketserver.h"
#include "qcorosignal.h"

#include <QWebSocket>
#include <QWebSocketServer>

using namespace QCoro::detail;

namespace {

class QCoroWebSocketServerSignalListener : public QObject {
    Q_OBJECT
public:
    QCoroWebSocketServerSignalListener(QWebSocketServer *server) {
        connect(server, &QWebSocketServer::closed, this, [this]() {
            Q_EMIT ready(nullptr);
        });
        connect(server, &QWebSocketServer::newConnection, [this, server]() {
            Q_EMIT ready(server->nextPendingConnection());
        });
    }

Q_SIGNALS:
    void ready(QWebSocket *socket);
};

} // namespace

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

    QCoroWebSocketServerSignalListener listener(server);
    const auto result = co_await qCoro(&listener, &QCoroWebSocketServerSignalListener::ready, timeout);
    co_return result.value_or(nullptr);
}

#include "qcorowebsocketserver.moc"
