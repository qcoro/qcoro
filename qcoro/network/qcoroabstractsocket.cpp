// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "qcoroabstractsocket.h"
#include "qcoroiodevice_p.h"
#include "qcorosignal.h"

using namespace QCoro::detail;
using namespace std::chrono_literals;

namespace {

class AbstractSocketReadySignalHelper : public WaitSignalHelper {
    Q_OBJECT
public:
    explicit AbstractSocketReadySignalHelper(const QAbstractSocket *socket, void(QIODevice::*readySignal)())
        : WaitSignalHelper(socket, readySignal)
        , mStateChanged(connect(socket, &QAbstractSocket::stateChanged, this,
                        [this](QAbstractSocket::SocketState state) { handleStateChange(state, false); }))
    {}

    explicit AbstractSocketReadySignalHelper(const QAbstractSocket *socket, void(QIODevice::*readySignal)(qint64))
        : WaitSignalHelper(socket, readySignal)
        , mStateChanged(connect(socket, &QAbstractSocket::stateChanged, this,
                        [this](QAbstractSocket::SocketState state) {
                            handleStateChange(state, static_cast<qint64>(0)); }))
    {}

private:
    template<typename T>
    void handleStateChange(QAbstractSocket::SocketState state, T result) {
        if (state == QAbstractSocket::ClosingState || state == QAbstractSocket::UnconnectedState) {
            disconnect(mStateChanged);
            emitReady(result);
        }
    }

private:
    QMetaObject::Connection mStateChanged;
};

} // namespace

QCoroAbstractSocket::QCoroAbstractSocket(QAbstractSocket *socket)
    : QCoroIODevice(socket) {}

QCoro::Task<std::optional<bool>> QCoroAbstractSocket::waitForReadyReadImpl(std::chrono::milliseconds timeout) {
    const auto *socket = static_cast<QAbstractSocket *>(mDevice.data());
    if (socket->state() != QAbstractSocket::ConnectedState) {
        co_return false;
    }

    AbstractSocketReadySignalHelper helper(socket, &QIODevice::readyRead);
    co_return co_await qCoro(&helper, qOverload<bool>(&WaitSignalHelper::ready), timeout);
}

QCoro::Task<std::optional<qint64>> QCoroAbstractSocket::waitForBytesWrittenImpl(std::chrono::milliseconds timeout) {
    const auto *socket = static_cast<QAbstractSocket *>(mDevice.data());
    if (socket->state() != QAbstractSocket::ConnectedState) {
        co_return std::nullopt;
    }

    AbstractSocketReadySignalHelper helper(socket, &QIODevice::bytesWritten);
    co_return co_await qCoro(&helper, qOverload<qint64>(&WaitSignalHelper::ready), timeout);
}

QCoro::Task<bool> QCoroAbstractSocket::waitForConnected(int timeout_msecs) {
    return waitForConnected(std::chrono::milliseconds{timeout_msecs});
}

QCoro::Task<bool> QCoroAbstractSocket::waitForConnected(std::chrono::milliseconds timeout) {
    const auto *socket = static_cast<QAbstractSocket *>(mDevice.data());
    if (socket->state() == QAbstractSocket::ConnectedState) {
        co_return true;
    }

    const auto result = co_await qCoro(socket, &QAbstractSocket::connected, timeout);
    co_return result.has_value();
}

QCoro::Task<bool> QCoroAbstractSocket::waitForDisconnected(int timeout_msecs) {
    return waitForDisconnected(std::chrono::milliseconds{timeout_msecs});
}

QCoro::Task<bool> QCoroAbstractSocket::waitForDisconnected(std::chrono::milliseconds timeout) {
    const auto *socket = static_cast<QAbstractSocket *>(mDevice.data());
    if (socket->state() == QAbstractSocket::UnconnectedState) {
        co_return false;
    }

    const auto result = co_await qCoro(socket, &QAbstractSocket::disconnected, timeout);
    co_return result.has_value();
}

QCoro::Task<bool> QCoroAbstractSocket::connectToHost(const QString &hostName, quint16 port,
                                                     QIODevice::OpenMode openMode, QAbstractSocket::NetworkLayerProtocol protocol,
                                                     std::chrono::milliseconds timeout) {
    static_cast<QAbstractSocket *>(mDevice.data())->connectToHost(hostName, port, openMode, protocol);
    return waitForConnected(timeout);
}

QCoro::Task<bool> QCoroAbstractSocket::connectToHost(const QHostAddress &address, quint16 port,
                                                     QIODevice::OpenMode openMode, std::chrono::milliseconds timeout) {
    static_cast<QAbstractSocket *>(mDevice.data())->connectToHost(address, port, openMode);
    return waitForConnected(timeout);
}

#include "qcoroabstractsocket.moc"