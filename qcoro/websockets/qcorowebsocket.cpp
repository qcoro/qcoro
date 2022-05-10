// SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "qcorowebsocket.h"
#include "qcorosignal.h"

#include <QWebSocket>

using namespace QCoro::detail;

using TupleQInt64QByteArray = std::tuple<qint64, QByteArray>;
using TupleQByteArrayBool = std::tuple<QByteArray, bool>;
using TupleQStringBool = std::tuple<QString, bool>;
Q_DECLARE_METATYPE(std::optional<TupleQInt64QByteArray>);
Q_DECLARE_METATYPE(std::optional<TupleQByteArrayBool>);
Q_DECLARE_METATYPE(std::optional<std::tuple<QByteArray>>);
Q_DECLARE_METATYPE(std::optional<TupleQStringBool>);
Q_DECLARE_METATYPE(std::optional<std::tuple<QString>>);

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
    WebSocketSignalWatcher(QWebSocket *socket, Signal &&signal)
        : mConnection(connect(socket, signal, this, [this](auto && ... args) {
            this->emitReady(std::make_optional(std::make_tuple(std::forward<decltype(args)>(args) ...)));
        }))
        , mState(connect(socket, &QWebSocket::stateChanged, this, [this](auto state) {
            // In theory, WebSocketSignalWatcher should never be used on
            // unconnected socket, so maybe the check is redundant
            if (state != QAbstractSocket::ConnectedState) {
                this->emitReady(std::optional<typename signal_args<Signal>::types>{});
            }
        }))
    {
        qRegisterMetaType<std::optional<std::tuple<qint64, QByteArray>>>();
        qRegisterMetaType<std::optional<std::tuple<QByteArray, bool>>>();
        qRegisterMetaType<std::optional<std::tuple<QByteArray>>>();
        qRegisterMetaType<std::optional<std::tuple<QString, bool>>>();
        qRegisterMetaType<std::optional<std::tuple<QString>>>();
    }

Q_SIGNALS:
    void ready(std::optional<std::tuple<qint64, QByteArray>>);
    void ready(std::optional<std::tuple<QByteArray, bool>>);
    void ready(std::optional<std::tuple<QByteArray>>);
    void ready(std::optional<std::tuple<QString, bool>>);
    void ready(std::optional<std::tuple<QString>>);

private:
    template<typename ... Args>
    void emitReady(std::optional<std::tuple<Args ...>> &&result) {
        disconnect(mConnection);
        disconnect(mState);

        Q_EMIT ready(std::forward<std::optional<std::tuple<Args ...>>>(result));
    }

    QMetaObject::Connection mConnection;
    QMetaObject::Connection mState;
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

QCoro::Task<std::optional<std::tuple<QByteArray, bool>>> QCoroWebSocket::binaryFrame(
    std::chrono::milliseconds timeout)
{
    if (mWebSocket->state() != QAbstractSocket::ConnectedState) {
        co_return std::nullopt;
    }

    WebSocketSignalWatcher watcher(mWebSocket, &QWebSocket::binaryFrameReceived);
    const auto result = co_await qCoro(&watcher, qOverload<std::optional<std::tuple<QByteArray, bool>>>(&WebSocketSignalWatcher::ready), timeout);
    co_return result.value_or(std::nullopt);
}

QCoro::Task<std::optional<QByteArray>> QCoroWebSocket::binaryMessage(std::chrono::milliseconds timeout)
{
    if (mWebSocket->state() != QAbstractSocket::ConnectedState) {
        co_return std::nullopt;
    }

    WebSocketSignalWatcher watcher(mWebSocket, &QWebSocket::binaryMessageReceived);
    auto result = co_await qCoro(&watcher, qOverload<std::optional<std::tuple<QByteArray>>>(&WebSocketSignalWatcher::ready) , timeout);
    if (result.has_value() && (*result).has_value()) {
        co_return std::get<0>(**result);
    }
    co_return std::nullopt;
}

QCoro::Task<std::optional<std::tuple<QString, bool>>> QCoroWebSocket::textFrame(
    std::chrono::milliseconds timeout)
{
    if (mWebSocket->state() != QAbstractSocket::ConnectedState) {
        co_return std::nullopt;
    }

    WebSocketSignalWatcher watcher(mWebSocket, &QWebSocket::textFrameReceived);
    const auto result = co_await qCoro(&watcher, qOverload<std::optional<std::tuple<QString, bool>>>(&WebSocketSignalWatcher::ready), timeout);
    co_return result.value_or(std::nullopt);
}

QCoro::Task<std::optional<QString>> QCoroWebSocket::textMessage(std::chrono::milliseconds timeout)
{
    if (mWebSocket->state() != QAbstractSocket::ConnectedState) {
        co_return std::nullopt;
    }

    WebSocketSignalWatcher watcher(mWebSocket, &QWebSocket::textMessageReceived);
    const auto result = co_await qCoro(&watcher, qOverload<std::optional<std::tuple<QString>>>(&WebSocketSignalWatcher::ready), timeout);
    if (result.has_value() && (*result).has_value()) {
        co_return std::get<0>(**result);
    }
    co_return std::nullopt;
}

#include "qcorowebsocket.moc"
