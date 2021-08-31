// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "qcorolocalsocket.h"

#include <chrono>
#include <functional>

using namespace QCoro::detail;

using namespace std::chrono_literals;

QCoroLocalSocket::WaitForConnectedOperation::WaitForConnectedOperation(QLocalSocket *socket, int timeout_msecs)
    : WaitOperationBase(socket, timeout_msecs)
{}

bool QCoroLocalSocket::WaitForConnectedOperation::await_ready() const noexcept {
    return !mObj || mObj->state() == QLocalSocket::ConnectedState;
}

void QCoroLocalSocket::WaitForConnectedOperation::await_suspend(QCORO_STD::coroutine_handle<> awaitingCoroutine) noexcept {
    mConn = QObject::connect(mObj, &QLocalSocket::stateChanged,
                             [this, awaitingCoroutine](auto newState) mutable {
                                 switch (newState) {
                                 case QLocalSocket::UnconnectedState:
                                 case QLocalSocket::ConnectingState:
                                     // Almost there...
                                     break;
                                 case QLocalSocket::ClosingState:
                                     // We shouldn't be here when waiting for Connected state...
                                     resume(awaitingCoroutine);
                                     break;
                                 case QLocalSocket::ConnectedState:
                                     resume(awaitingCoroutine);
                                     break;
                                 }
                             });

    startTimeoutTimer(awaitingCoroutine);
}

QCoroLocalSocket::WaitForDisconnectedOperation::WaitForDisconnectedOperation(QLocalSocket *socket, int timeout_msecs)
    : WaitOperationBase(socket, timeout_msecs)
{}

bool QCoroLocalSocket::WaitForDisconnectedOperation::await_ready() const noexcept {
    return !mObj || mObj->state() == QLocalSocket::UnconnectedState;
}

void QCoroLocalSocket::WaitForDisconnectedOperation::await_suspend(QCORO_STD::coroutine_handle<> awaitingCoroutine) {
    mConn = QObject::connect(mObj, &QLocalSocket::disconnected,
                             std::bind(&WaitForDisconnectedOperation::resume, this, awaitingCoroutine));
    startTimeoutTimer(awaitingCoroutine);
}

bool QCoroLocalSocket::ReadOperation::await_ready() const noexcept {
    return QCoroIODevice::ReadOperation::await_ready() ||
           static_cast<const QLocalSocket *>(mDevice.data())->state() == QLocalSocket::UnconnectedState;
}

void QCoroLocalSocket::ReadOperation::await_suspend(QCORO_STD::coroutine_handle<> awaitingCoroutine) noexcept {
    QCoroIODevice::ReadOperation::await_suspend(awaitingCoroutine);
    mStateConn = QObject::connect(
        static_cast<QLocalSocket *>(mDevice.data()), &QLocalSocket::stateChanged,
        [this, awaitingCoroutine]() {
            if (static_cast<const QLocalSocket *>(mDevice.data())->state() == QLocalSocket::UnconnectedState) {
                finish(awaitingCoroutine);
            }
        });
}

void QCoroLocalSocket::ReadOperation::finish(QCORO_STD::coroutine_handle<> awaitingCoroutine) {
    QObject::disconnect(mStateConn);
    QCoroIODevice::ReadOperation::finish(awaitingCoroutine);
}


QCoroLocalSocket::QCoroLocalSocket(QLocalSocket *socket)
    : QCoroIODevice(socket)
{}

QCoroLocalSocket::WaitForConnectedOperation QCoroLocalSocket::waitForConnected(int timeout_msecs) {
    return WaitForConnectedOperation{static_cast<QLocalSocket *>(mDevice.data()), timeout_msecs};
}

QCoroLocalSocket::WaitForConnectedOperation QCoroLocalSocket::waitForConnected(std::chrono::milliseconds timeout) {
    return waitForConnected(static_cast<int>(timeout.count()));
}

QCoroLocalSocket::WaitForDisconnectedOperation QCoroLocalSocket::waitForDisconnected(int timeout_msecs) {
    return WaitForDisconnectedOperation{static_cast<QLocalSocket *>(mDevice.data()), timeout_msecs};
}

QCoroLocalSocket::WaitForDisconnectedOperation QCoroLocalSocket::waitForDisconnected(std::chrono::milliseconds timeout) {
    return waitForDisconnected(static_cast<int>(timeout.count()));
}

QCoroLocalSocket::WaitForConnectedOperation QCoroLocalSocket::connectToServer(QIODevice::OpenMode openMode) {
    static_cast<QLocalSocket *>(mDevice.data())->connectToServer(openMode);
    return waitForConnected();
}

QCoroLocalSocket::WaitForConnectedOperation QCoroLocalSocket::connectToServer(const QString &name,
                                                                              QIODevice::OpenMode openMode) {
    static_cast<QLocalSocket *>(mDevice.data())->connectToServer(name, openMode);
    return waitForConnected();
}

QCoroLocalSocket::ReadOperation QCoroLocalSocket::readAll() {
    return ReadOperation(mDevice, [](QIODevice *dev) { return dev->readAll(); });
}

//! \copydoc QIODevice::read
QCoroLocalSocket::ReadOperation QCoroLocalSocket::read(qint64 maxSize) {
    return ReadOperation(mDevice, [maxSize](QIODevice *dev) { return dev->read(maxSize); });
}

//! \copydoc QIODevice::readLine
QCoroLocalSocket::ReadOperation QCoroLocalSocket::readLine(qint64 maxSize) {
    return ReadOperation(mDevice, [maxSize](QIODevice *dev) { return dev->readLine(maxSize); });
}
