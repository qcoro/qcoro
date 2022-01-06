// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "qcoroabstractsocket.h"

using namespace QCoro::detail;
using namespace std::chrono_literals;

QCoroAbstractSocket::WaitForConnectedOperation::WaitForConnectedOperation(QAbstractSocket *socket, int timeout_msecs)
    : WaitOperationBase<QAbstractSocket>(socket, timeout_msecs)
{}

bool QCoroAbstractSocket::WaitForConnectedOperation::await_ready() const noexcept {
    return !mObj || mObj->state() == QAbstractSocket::ConnectedState;
}

void QCoroAbstractSocket::WaitForConnectedOperation::await_suspend(std::coroutine_handle<> awaitingCoroutine) noexcept {
    mConn = QObject::connect(mObj, &QAbstractSocket::stateChanged,
                             [this, awaitingCoroutine](auto newState) mutable {
                                 switch (newState) {
                                 case QAbstractSocket::UnconnectedState:
                                 case QAbstractSocket::HostLookupState:
                                 case QAbstractSocket::ConnectingState:
                                 case QAbstractSocket::BoundState:
                                     // Almost there...
                                     break;
                                 case QAbstractSocket::ClosingState:
                                 case QAbstractSocket::ListeningState:
                                     // We shouldn't be here when waiting for Connected state...
                                     resume(awaitingCoroutine);
                                     break;
                                 case QAbstractSocket::ConnectedState:
                                     resume(awaitingCoroutine);
                                     break;
                                 }
                             });

    startTimeoutTimer(awaitingCoroutine);
}

QCoroAbstractSocket::WaitForDisconnectedOperation::WaitForDisconnectedOperation(QAbstractSocket *socket, int timeout_msecs)
    : WaitOperationBase(socket, timeout_msecs) {}

bool QCoroAbstractSocket::WaitForDisconnectedOperation::await_ready() const noexcept {
    return !mObj || mObj->state() == QAbstractSocket::UnconnectedState;
}

void QCoroAbstractSocket::WaitForDisconnectedOperation::await_suspend(std::coroutine_handle<> awaitingCoroutine) noexcept{
    mConn = QObject::connect(
        mObj, &QAbstractSocket::disconnected,
        [this, awaitingCoroutine]() mutable { resume(awaitingCoroutine); });
    startTimeoutTimer(awaitingCoroutine);
}

bool QCoroAbstractSocket::ReadOperation::await_ready() const noexcept {
    return QCoroIODevice::ReadOperation::await_ready() ||
           static_cast<const QAbstractSocket *>(mDevice.data())->state() ==
               QAbstractSocket::UnconnectedState;
}

void QCoroAbstractSocket::ReadOperation::await_suspend(std::coroutine_handle<> awaitingCoroutine) noexcept {
    QCoroIODevice::ReadOperation::await_suspend(awaitingCoroutine);
    mStateConn = QObject::connect(
        static_cast<QAbstractSocket *>(mDevice.data()), &QAbstractSocket::stateChanged,
        [this, awaitingCoroutine]() {
            if (static_cast<const QAbstractSocket *>(mDevice.data())->state() ==
                QAbstractSocket::UnconnectedState) {
                finish(awaitingCoroutine);
            }
        });
}

void QCoroAbstractSocket::ReadOperation::finish(std::coroutine_handle<> awaitingCoroutine) {
    QObject::disconnect(mStateConn);
    QCoroIODevice::ReadOperation::finish(awaitingCoroutine);
}


QCoroAbstractSocket::QCoroAbstractSocket(QAbstractSocket *socket)
    : QCoroIODevice(socket) {}

QCoroAbstractSocket::WaitForConnectedOperation QCoroAbstractSocket::waitForConnected(int timeout_msecs) {
    return WaitForConnectedOperation{static_cast<QAbstractSocket *>(mDevice.data()), timeout_msecs};
}

QCoroAbstractSocket::WaitForConnectedOperation QCoroAbstractSocket::waitForConnected(std::chrono::milliseconds timeout) {
    return waitForConnected(static_cast<int>(timeout.count()));
}

QCoroAbstractSocket::WaitForDisconnectedOperation QCoroAbstractSocket::waitForDisconnected(int timeout_msecs) {
    return WaitForDisconnectedOperation{static_cast<QAbstractSocket *>(mDevice.data()), timeout_msecs};
}

QCoroAbstractSocket::WaitForDisconnectedOperation QCoroAbstractSocket::waitForDisconnected(std::chrono::milliseconds timeout) {
    return waitForDisconnected(static_cast<int>(timeout.count()));
}

QCoroAbstractSocket::WaitForConnectedOperation QCoroAbstractSocket::connectToHost(const QString &hostName, quint16 port,
              QIODevice::OpenMode openMode, QAbstractSocket::NetworkLayerProtocol protocol) {
    static_cast<QAbstractSocket *>(mDevice.data())->connectToHost(hostName, port, openMode, protocol);
    return waitForConnected();
}

QCoroAbstractSocket::WaitForConnectedOperation QCoroAbstractSocket::connectToHost(const QHostAddress &address, quint16 port,
                             QIODevice::OpenMode openMode) {
    static_cast<QAbstractSocket *>(mDevice.data())->connectToHost(address, port, openMode);
    return waitForConnected();
}

QCoroAbstractSocket::ReadOperation QCoroAbstractSocket::readAll() {
    return ReadOperation(mDevice, [](QIODevice *dev) { return dev->readAll(); });
}

QCoroAbstractSocket::ReadOperation QCoroAbstractSocket::read(qint64 maxSize) {
    return ReadOperation(mDevice, [maxSize](QIODevice *dev) { return dev->read(maxSize); });
}

QCoroAbstractSocket::ReadOperation QCoroAbstractSocket::readLine(qint64 maxSize) {
    return ReadOperation(mDevice, [maxSize](QIODevice *dev) { return dev->readLine(maxSize); });
}

