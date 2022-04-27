// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "qcorolocalsocket.h"
#include "qcorosignal.h"
#include "qcoroiodevice_p.h"

#include <chrono>
#include <functional>

using namespace QCoro::detail;

using namespace std::chrono_literals;

namespace {

class SocketConnectedHelper : public QObject {
    Q_OBJECT
public:
    SocketConnectedHelper(const QLocalSocket *socket, void(QLocalSocket::*signal)())
        : QObject()
        , mSignal(connect(socket, signal, this, [this]() { emitReady(true); }))
        , mStateChange(connect(socket, &QLocalSocket::stateChanged, this, [this](auto state) {
            if (state == QLocalSocket::UnconnectedState) {
                emitReady(false);
            }
        }))
    {}

Q_SIGNALS:
    void ready(bool result);

private:
    void emitReady(bool result) {
        disconnect(mSignal);
        disconnect(mStateChange);
        Q_EMIT ready(result);
    }
private:
    QMetaObject::Connection mSignal;
    QMetaObject::Connection mStateChange;
};

class LocalSocketReadySignalHelper : public WaitSignalHelper {
    Q_OBJECT
public:
    LocalSocketReadySignalHelper(const QLocalSocket *socket, void(QIODevice::*signal)())
        : WaitSignalHelper(socket, signal)
        , mStateChange(connect(socket, &QLocalSocket::stateChanged, this,
                               [this](auto state) { stateChanged(state, false); }))
    {}
    LocalSocketReadySignalHelper(const QLocalSocket *socket, void(QIODevice::*signal)(qint64))
        : WaitSignalHelper(socket, signal)
        , mStateChange(connect(socket, &QLocalSocket::stateChanged, this,
                               [this](auto state) { stateChanged(state, static_cast<qint64>(0)); }))
    {}

private:
    template<typename T>
    void stateChanged(QLocalSocket::LocalSocketState state, T result) {
        if (state != QLocalSocket::ConnectedState) {
            disconnect(mStateChange);
            emitReady(result);
        }
    }

private:
    QMetaObject::Connection mStateChange;
};

} // namespace

QCoroLocalSocket::QCoroLocalSocket(QLocalSocket *socket)
    : QCoroIODevice(socket)
{}

QCoro::Task<std::optional<bool>> QCoroLocalSocket::waitForReadyReadImpl(std::chrono::milliseconds timeout) {
    const auto *socket = static_cast<QLocalSocket *>(mDevice.data());
    if (socket->state() != QLocalSocket::ConnectedState) {
        co_return false;
    }
    LocalSocketReadySignalHelper helper(socket, &QLocalSocket::readyRead);
    co_return co_await qCoro(&helper, qOverload<bool>(&LocalSocketReadySignalHelper::ready), timeout);
}

QCoro::Task<std::optional<qint64>> QCoroLocalSocket::waitForBytesWrittenImpl(std::chrono::milliseconds timeout) {
    const auto *socket = static_cast<QLocalSocket *>(mDevice.data());
    if (socket->state() != QLocalSocket::ConnectedState) {
        co_return std::nullopt;
    }
    LocalSocketReadySignalHelper helper(socket, &QLocalSocket::bytesWritten);
    co_return co_await qCoro(&helper, qOverload<qint64>(&LocalSocketReadySignalHelper::ready), timeout);
}

QCoro::Task<bool> QCoroLocalSocket::waitForConnected(int timeout_msecs) {
    return waitForConnected(std::chrono::milliseconds(timeout_msecs));
}

QCoro::Task<bool> QCoroLocalSocket::waitForConnected(std::chrono::milliseconds timeout) {
    const auto *socket = static_cast<QLocalSocket *>(mDevice.data());
    if (socket->state() == QLocalSocket::ConnectedState) {
        co_return true;
    }
    SocketConnectedHelper helper(socket, &QLocalSocket::connected);
    const auto result = co_await qCoro(&helper, &SocketConnectedHelper::ready, timeout);
    co_return result.value_or(false);
}

QCoro::Task<bool> QCoroLocalSocket::waitForDisconnected(int timeout_msecs) {
    return waitForDisconnected(std::chrono::milliseconds(timeout_msecs));
}

QCoro::Task<bool> QCoroLocalSocket::waitForDisconnected(std::chrono::milliseconds timeout) {
    const auto *socket = static_cast<QLocalSocket *>(mDevice.data());
    if (socket->state() == QLocalSocket::UnconnectedState) {
        co_return false;
    }
    const auto result = co_await qCoro(socket, &QLocalSocket::disconnected, timeout);
    co_return result.has_value();
}

QCoro::Task<bool> QCoroLocalSocket::connectToServer(QIODevice::OpenMode openMode,
                                                    std::chrono::milliseconds timeout) {
    static_cast<QLocalSocket *>(mDevice.data())->connectToServer(openMode);
    return waitForConnected(timeout);
}

QCoro::Task<bool> QCoroLocalSocket::connectToServer(const QString &name, QIODevice::OpenMode openMode,
                                                    std::chrono::milliseconds timeout) {
    static_cast<QLocalSocket *>(mDevice.data())->connectToServer(name, openMode);
    return waitForConnected(timeout);
}

#include "qcorolocalsocket.moc"