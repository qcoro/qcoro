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

QCoroIODevice::QCoroIODevice(QIODevice *device)
    : mDevice{device}
{}

QCoroIODevice::ReadOperation QCoroIODevice::readAll() {
    return ReadOperation(mDevice, [](QIODevice *dev) { return dev->readAll(); });
}

QCoroIODevice::ReadOperation QCoroIODevice::read(qint64 maxSize) {
    return ReadOperation(mDevice, [maxSize](QIODevice *dev) { return dev->read(maxSize); });
}

QCoroIODevice::ReadOperation QCoroIODevice::readLine(qint64 maxSize) {
    return ReadOperation(mDevice, [maxSize](QIODevice *dev) { return dev->readLine(maxSize); });
}

QCoroIODevice::WriteOperation QCoroIODevice::write(const QByteArray &buffer) {
    return WriteOperation(mDevice, buffer);
}

