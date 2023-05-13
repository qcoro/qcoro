// SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "qcorowebsocket.h"
#include "qcoroasyncgenerator.h"
#include "qcorosignal.h"

#include <QWebSocket>
#include <QDebug>

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
        , mError(connect(socket, qOverload<QAbstractSocket::SocketError>(
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
                                     &QWebSocket::errorOccurred
#else
                                     &QWebSocket::error
#endif
                                     ), this, [this](auto error) {
            qWarning() << "QWebSocket failed to connect to a websocket server: " << error;
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
    using types = std::tuple<std::remove_cvref_t<Args> ...>;
};

template<typename T>
using signal_args_t = typename signal_args<T>::types;

template<typename>
struct unwrapped_signal_args;

template<typename ... Args>
struct unwrapped_signal_args<std::tuple<Args ...>> {
private:
    using args_tuple = std::tuple<std::remove_cvref_t<Args> ...>;
public:
    using type = std::conditional_t<std::tuple_size_v<args_tuple> == 1,
                                    std::tuple_element_t<0, args_tuple>,
                                    args_tuple>;
};

template<typename ... Args>
using unwrapped_signal_args_t = typename unwrapped_signal_args<Args ...>::type;

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
                Q_EMIT this->ready(std::optional<signal_args_t<Signal>>{});
            }
        });
    }

Q_SIGNALS:
    void ready(std::optional<std::tuple<qint64, QByteArray>>); // ping
    void ready(std::optional<std::tuple<QByteArray, bool>>); // binary frames
    void ready(std::optional<std::tuple<QByteArray>>); // binary messages
    void ready(std::optional<std::tuple<QString, bool>>); // text frames
    void ready(std::optional<std::tuple<QString>>); // text messages
};

template<typename Signal>
auto watcherGenerator(QWebSocket *ws, Signal signal, std::chrono::milliseconds timeout) ->
    QCoro::AsyncGenerator<unwrapped_signal_args_t<signal_args_t<Signal>>>
{
    WebSocketSignalWatcher watcher(ws, signal);
    using signalType = std::optional<signal_args_t<Signal>>;
    auto signalListener = qCoroSignalListener(&watcher, qOverload<signalType>(&WebSocketSignalWatcher::ready), timeout);
    auto it = co_await signalListener.begin();
    while (it != signalListener.end()) {
        if (!(*it).has_value()) {
            break;
        }
        // If the signal is a single-value tuple, we unwrap it from the tuple, otherwise we yield the whole tuple.
        if constexpr (std::tuple_size_v<typename signalType::value_type> == 1) {
            co_yield std::get<0>(**it);
        } else {
            co_yield **it;
        }
        co_await ++it;
    }
}


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

QCoro::Task<std::optional<std::chrono::milliseconds>> QCoroWebSocket::ping(const QByteArray &payload,
                                                                           std::chrono::milliseconds timeout)
{
    if (mWebSocket->state() != QAbstractSocket::ConnectedState) {
        co_return std::nullopt;
    }

    WebSocketSignalWatcher watcher(mWebSocket, &QWebSocket::pong);
    mWebSocket->ping(payload);
    const auto result = co_await qCoro(&watcher, qOverload<std::optional<std::tuple<qint64, QByteArray>>>(&WebSocketSignalWatcher::ready), timeout);
    if (result.has_value() && (*result).has_value()) {
        co_return std::chrono::milliseconds{std::get<0>(**result)};
    }
    co_return std::nullopt;
}

QCoro::AsyncGenerator<std::tuple<QByteArray, bool>> QCoroWebSocket::binaryFrames(
    std::chrono::milliseconds timeout)
{
    return watcherGenerator(mWebSocket, &QWebSocket::binaryFrameReceived, timeout);
}

QCoro::AsyncGenerator<QByteArray> QCoroWebSocket::binaryMessages(
    std::chrono::milliseconds timeout)
{
    return watcherGenerator(mWebSocket, &QWebSocket::binaryMessageReceived, timeout);
}

QCoro::AsyncGenerator<std::tuple<QString, bool>> QCoroWebSocket::textFrames(
    std::chrono::milliseconds timeout)
{
    return watcherGenerator(mWebSocket, &QWebSocket::textFrameReceived, timeout);
}

QCoro::AsyncGenerator<QString> QCoroWebSocket::textMessages(
    std::chrono::milliseconds timeout)
{
    return watcherGenerator(mWebSocket, &QWebSocket::textMessageReceived, timeout);
}

#include "qcorowebsocket.moc"
