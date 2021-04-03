// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "../macros.h"

#include <QPointer>
#include <QIODevice>
#include <QByteArray>

namespace QCoro::detail {

template<typename ResultCb>
class QIODeviceReadOperation {
public:
    QIODeviceReadOperation(QIODevice *device, ResultCb &&resultCb)
        : mDevice(device), mResultCb(resultCb) {}

    Q_DISABLE_COPY(QIODeviceReadOperation);
    QCORO_DEFAULT_MOVE(QIODeviceReadOperation);

    ~QIODeviceReadOperation() = default;

    bool await_ready() const noexcept {
        return !mDevice || !mDevice->isOpen() || !mDevice->isReadable() || mDevice->bytesAvailable() > 0;
    }

    void await_suspend(QCORO_STD::coroutine_handle<> awaitingCoroutine) noexcept {
        Q_ASSERT(mDevice);
        mConn = QObject::connect(mDevice, &QIODevice::readyRead,
                                [this, awaitingCoroutine]() mutable {
                                    QObject::disconnect(mConn);
                                    awaitingCoroutine.resume();
                                });
    }

    auto await_resume() {
        return mResultCb(mDevice);
    }

private:
    QPointer<QIODevice> mDevice;
    ResultCb mResultCb;
    QMetaObject::Connection mConn;
};

class QIODeviceWriteOperation {
public:
    QIODeviceWriteOperation(QIODevice *device, const QByteArray &data)
        : mDevice(device), mBytesToBeWritten(device->write(data)) {}

    Q_DISABLE_COPY(QIODeviceWriteOperation);
    QCORO_DEFAULT_MOVE(QIODeviceWriteOperation);

    ~QIODeviceWriteOperation() = default;

    bool await_ready() const noexcept {
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

    void await_suspend(QCORO_STD::coroutine_handle<> awaitingCoroutine) noexcept {
        Q_ASSERT(mDevice);
        mConn = QObject::connect(mDevice, &QIODevice::bytesWritten,
                                 [this, awaitingCoroutine](qint64 written) mutable {
                                    mBytesWritten += written;
                                    if (mBytesWritten >= mBytesToBeWritten) {
                                        QObject::disconnect(mConn);
                                        awaitingCoroutine.resume();
                                    }
                                });
    }

    qint64 await_resume() noexcept {
        return mBytesWritten;
    }


private:
    QPointer<QIODevice> mDevice;
    QMetaObject::Connection mConn;
    qint64 mBytesToBeWritten = 0;
    qint64 mBytesWritten = 0;
};


class QIODeviceWrapper {
public:
    explicit QIODeviceWrapper(QIODevice *device)
        : mDevice{device}
    {}

    auto readAll() {
        return QIODeviceReadOperation(mDevice, [](QIODevice *dev) { return dev->readAll(); });
    }

    auto read(qint64 maxSize) {
        return QIODeviceReadOperation(mDevice, [maxSize](QIODevice *dev) { return dev->read(maxSize); });
    }

    auto readLine(qint64 maxSize = 0) {
        return QIODeviceReadOperation(mDevice, [maxSize](QIODevice *dev) { return dev->readLine(maxSize); });
    }

    // TODO
    //auto bytesAvailable(qint64 minBytes) {

    auto write(const QByteArray &buffer) {
        return QIODeviceWriteOperation(mDevice, buffer);
    }

protected:
    QPointer<QIODevice> mDevice = {};
};

} /// namespace QCoro::detail
