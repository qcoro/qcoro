// SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "qcorowebsocket.h"
#include "qcorosignal.h"

#include <QWebSocket>

using namespace QCoro::detail;

namespace {

class WebSocketStateWatcher : public QObject {
    Q_OBJECT
public:
    WebSocketStateWatcher(QWebSocket *socket, QAbstractSocket::SocketState desiredState)
        : mState(connect(socket, &QWebSocket::stateChanged, this, [this, desiredState](auto newState) {
            if (newState == desiredState) {
                emitReady(true);
            }
        }))
        , mError(connect(socket, qOverload<QAbstractSocket::SocketError>(&QWebSocket::error), this, [this]() {
            emitReady(false);
        }))
    {}

Q_SIGNALS:
    void ready(bool result);

private:
    void emitReady(bool result) {
        disconnect(mState);
        disconnect(mError);
        Q_EMIT ready(result);
    }

    QMetaObject::Connection mState;
    QMetaObject::Connection mError;
};

} // namespace

QCoroWebSocket::QCoroWebSocket(QWebSocket *socket)
    : mWebSocket(socket)
{}

QCoro::Task<bool> QCoroWebSocket::open(const QUrl &url, std::chrono::milliseconds timeout)
{
    if (mWebSocket->state() == QAbstractSocket::ConnectedState) {
        co_return true;
    }

    WebSocketStateWatcher watcher(mWebSocket, QAbstractSocket::ConnectedState);
    mWebSocket->open(url);
    const auto result = co_await qCoro(&watcher, &WebSocketStateWatcher::ready, timeout);
    co_return result.value_or(false);
}

QCoro::Task<bool> QCoroWebSocket::open(const QNetworkRequest &request, std::chrono::milliseconds timeout)
{
    if (mWebSocket->state() == QAbstractSocket::ConnectedState) {
        co_return true;
    }

    WebSocketStateWatcher watcher(mWebSocket, QAbstractSocket::ConnectedState);
    mWebSocket->open(request);
    const auto result = co_await qCoro(&watcher, &WebSocketStateWatcher::ready, timeout);
    co_return result.value_or(false);
}

QCoro::Task<bool> QCoroWebSocket::close(QWebSocketProtocol::CloseCode closeCode,
                                        const QString &reason,
                                        std::chrono::milliseconds timeout)
{
    if (mWebSocket->state() == QAbstractSocket::UnconnectedState) {
        co_return true;
    }

    WebSocketStateWatcher watcher(mWebSocket, QAbstractSocket::UnconnectedState);
    mWebSocket->close(closeCode,reason);
    const auto result = co_await qCoro(&watcher, &WebSocketStateWatcher::ready, timeout);
    co_return result.value_or(false);
}

#include "qcorowebsocket.moc"