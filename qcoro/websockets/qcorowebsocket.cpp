// SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "qcorowebsocket.h"
#include "task.h"
#include "qcoroasyncgenerator.h"
#include "qcorosignal.h"

#include <QWebSocket>

using namespace QCoro::detail;

using TupleQInt64QByteArray = std::tuple<qint64, QByteArray>;
using TupleQByteArrayBool = std::tuple<QByteArray, bool>;
using TupleQStringBool = std::tuple<QString, bool>;

Q_DECLARE_METATYPE(std::optional<TupleQInt64QByteArray>)
Q_DECLARE_METATYPE(std::optional<TupleQByteArrayBool>)
Q_DECLARE_METATYPE(std::optional<std::tuple<QByteArray>>)
Q_DECLARE_METATYPE(std::optional<TupleQStringBool>)
Q_DECLARE_METATYPE(std::optional<std::tuple<QString>>)

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

template<typename>
struct signal_args;

template<typename T, typename R, typename ... Args>
struct signal_args<R(T::*)(Args ...)> {
    using types = std::tuple<Args ...>;
};

class WebSocketSignalWatcher : public QObject {
    Q_OBJECT
public:
    template<typename Signal>
    WebSocketSignalWatcher(QWebSocket *socket, Signal signal) {
        qRegisterMetaType<std::optional<std::tuple<qint64, QByteArray>>>();
        qRegisterMetaType<std::optional<std::tuple<QByteArray, bool>>>();
        qRegisterMetaType<std::optional<std::tuple<QByteArray>>>();
        qRegisterMetaType<std::optional<std::tuple<QString, bool>>>();
        qRegisterMetaType<std::optional<std::tuple<QString>>>();

        connect(socket, signal, this, [this](auto && ... args) {
            Q_EMIT this->ready(std::make_optional(std::make_tuple(std::forward<decltype(args)>(args) ...)));
        });
        connect(socket, &QWebSocket::stateChanged, this, [this](auto state) {
            // In theory, WebSocketSignalWatcher should never be used on
            // unconnected socket, so maybe the check is redundant
            if (state != QAbstractSocket::ConnectedState) {
                Q_EMIT this->ready(std::optional<typename signal_args<Signal>::types>{});
            }
        });
    }

Q_SIGNALS:
    void ready(std::optional<std::tuple<qint64, QByteArray>>);
    void ready(std::optional<std::tuple<QByteArray, bool>>);
    void ready(std::optional<std::tuple<QByteArray>>);
    void ready(std::optional<std::tuple<QString, bool>>);
    void ready(std::optional<std::tuple<QString>>);
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

QCoro::Task<std::optional<qint64>> QCoroWebSocket::ping(const QByteArray &payload,
                                                        std::chrono::milliseconds timeout)
{
    if (mWebSocket->state() != QAbstractSocket::ConnectedState) {
        co_return std::nullopt;
    }

    WebSocketSignalWatcher watcher(mWebSocket, &QWebSocket::pong);
    mWebSocket->ping(payload);
    const auto result = co_await qCoro(&watcher, qOverload<std::optional<std::tuple<qint64, QByteArray>>>(&WebSocketSignalWatcher::ready), timeout);
    if (result.has_value() && (*result).has_value()) {
        co_return std::get<0>(**result);
    }
    co_return std::nullopt;
}

QCoro::AsyncGenerator<std::tuple<QByteArray, bool>> QCoroWebSocket::binaryFrames(
    std::chrono::milliseconds timeout)
{
    return qCoroSignalGenerator(mWebSocket, &QWebSocket::binaryFrameReceived, timeout);
}

QCoro::AsyncGenerator<QByteArray> QCoroWebSocket::binaryMessages(
    std::chrono::milliseconds timeout)
{
    return qCoroSignalGenerator(mWebSocket, &QWebSocket::binaryMessageReceived, timeout);
}

QCoro::AsyncGenerator<std::tuple<QString, bool>> QCoroWebSocket::textFrames(
    std::chrono::milliseconds timeout)
{
    return qCoroSignalGenerator(mWebSocket, &QWebSocket::textFrameReceived, timeout);
}

QCoro::AsyncGenerator<QString> QCoroWebSocket::textMessages(
    std::chrono::milliseconds timeout)
{
    return qCoroSignalGenerator(mWebSocket, &QWebSocket::textMessageReceived, timeout);
}

#include "qcorowebsocket.moc"
