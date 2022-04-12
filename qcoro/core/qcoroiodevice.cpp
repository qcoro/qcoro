// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "qcoroiodevice.h"

#include <QByteArray>
#include <QIODevice>
#include <QTimer>

using namespace QCoro::detail;

QCoroIODevice::OperationBase::OperationBase(QIODevice *device)
    : mDevice(device)
{}

void QCoroIODevice::OperationBase::finish(std::coroutine_handle<> awaitingCoroutine) {
    QObject::disconnect(mConn);
    QObject::disconnect(mCloseConn);
    // Delayed trigger
    QTimer::singleShot(0, [awaitingCoroutine]() mutable { awaitingCoroutine.resume(); });
}

QCoroIODevice::ReadOperation::ReadOperation(QIODevice *device, std::function<QByteArray(QIODevice *)> &&resultCb)
    : OperationBase(device), mResultCb(std::move(resultCb)) {}

bool QCoroIODevice::ReadOperation::await_ready() const noexcept {
    return !mDevice || !mDevice->isOpen() || !mDevice->isReadable() ||
           mDevice->bytesAvailable() > 0;
}

void QCoroIODevice::ReadOperation::await_suspend(std::coroutine_handle<> awaitingCoroutine) noexcept {
    Q_ASSERT(mDevice);
    mConn = QObject::connect(mDevice, &QIODevice::readyRead,
                             std::bind(&ReadOperation::finish, this, awaitingCoroutine));
    mCloseConn =
        QObject::connect(mDevice, &QIODevice::aboutToClose,
                         std::bind(&ReadOperation::finish, this, awaitingCoroutine));
}

QByteArray QCoroIODevice::ReadOperation::await_resume() {
    return mResultCb(mDevice);
}

QCoroIODevice::WriteOperation::WriteOperation(QIODevice *device, const QByteArray &data)
    : OperationBase(device), mBytesToBeWritten(device->write(data))
{}

bool QCoroIODevice::WriteOperation::await_ready() const noexcept {
    if (!mDevice || !mDevice->isOpen() || !mDevice->isWritable()) {
        return true;
    }

    if (mBytesWritten == 0) {
        return true;
    }

    if (mDevice->bytesToWrite() == 0) {
        return true;
    }

    return false;
}

void QCoroIODevice::WriteOperation::await_suspend(std::coroutine_handle<> awaitingCoroutine) noexcept {
    Q_ASSERT(mDevice);
    mConn = QObject::connect(mDevice, &QIODevice::bytesWritten,
                             [this, awaitingCoroutine](qint64 written) {
                                 mBytesWritten += written;
                                 if (mBytesWritten >= mBytesToBeWritten) {
                                     finish(awaitingCoroutine);
                                 }
                             });
    mCloseConn = QObject::connect(mDevice, &QIODevice::aboutToClose,
                                  std::bind(&WriteOperation::finish, this, awaitingCoroutine));
}

qint64 QCoroIODevice::WriteOperation::await_resume() noexcept {
    return mBytesWritten;
}

QCoroIODevice::ReadAllOperation::ReadAllOperation(QIODevice *device)
    : ReadOperation(device, [](QIODevice *d) { return d->readAll(); }) {}

QCoroIODevice::ReadAllOperation::ReadAllOperation(QIODevice &device)
    : ReadAllOperation(&device) {}


QCoroIODevice::WaitForReadyReadOperation::WaitForReadyReadOperation(QIODevice *device, int timeout_msecs)
    : WaitOperationBase(device, timeout_msecs) {}

bool QCoroIODevice::WaitForReadyReadOperation::await_ready() const noexcept {
    return mObj->waitForReadyRead(0);
}

void QCoroIODevice::WaitForReadyReadOperation::await_suspend(std::coroutine_handle<> awaitingCoroutine) {
    mConn = QObject::connect(mObj, &QIODevice::readyRead,
                             [this, awaitingCoroutine]() {
                                resume(awaitingCoroutine);
                             });
}

QCoroIODevice::WaitForBytesWrittenOperation::WaitForBytesWrittenOperation(QIODevice *device, int timeout_msecs)
    : WaitOperationBase(device, timeout_msecs) {}

bool QCoroIODevice::WaitForBytesWrittenOperation::await_ready() const noexcept {
    return mObj->waitForBytesWritten(0);
}

void QCoroIODevice::WaitForBytesWrittenOperation::await_suspend(std::coroutine_handle<> awaitingCoroutine) {
    mConn = QObject::connect(mObj, &QIODevice::bytesWritten,
                             [this, awaitingCoroutine]() {
                                resume(awaitingCoroutine);
                            });
}

QCoroIODevice::QCoroIODevice(QIODevice *device)
    : mDevice{device}
{}

QCoro::Task<QByteArray> QCoroIODevice::readAll(std::chrono::milliseconds timeout) {
    if (!co_await waitForReadyRead(timeout)) {
        co_return QByteArray{};
    }

    co_return mDevice->readAll();
}

QCoro::Task<QByteArray> QCoroIODevice::read(qint64 maxSize, std::chrono::milliseconds timeout) {
    if (!co_await waitForReadyRead(timeout)) {
        co_return QByteArray{};
    }

    co_return mDevice->read(maxSize);
}

QCoro::Task<QByteArray> QCoroIODevice::readLine(qint64 maxSize, std::chrono::milliseconds timeout) {
    if (!co_await waitForReadyRead(timeout)) {
        co_return QByteArray{};
    }

    co_return mDevice->readLine(maxSize);
}

QCoroIODevice::WriteOperation QCoroIODevice::write(const QByteArray &buffer) {
    return WriteOperation(mDevice, buffer);
}

QCoroIODevice::WaitForReadyReadOperation QCoroIODevice::waitForReadyRead(int timeout_msecs) {
    return WaitForReadyReadOperation(mDevice, timeout_msecs);
}

QCoroIODevice::WaitForReadyReadOperation QCoroIODevice::waitForReadyRead(std::chrono::milliseconds timeout) {
    return waitForReadyRead(timeout.count());
}

QCoroIODevice::WaitForBytesWrittenOperation QCoroIODevice::waitForBytesWritten(int timeout_msecs) {
    return WaitForBytesWrittenOperation(mDevice, timeout_msecs);
}

QCoroIODevice::WaitForBytesWrittenOperation QCoroIODevice::waitForBytesWritten(std::chrono::milliseconds timeout) {
    return waitForBytesWritten(timeout.count());
}

