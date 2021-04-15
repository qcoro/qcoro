// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "coroutine.h"
#include "macros.h"

#include <QPointer>
#include <QIODevice>
#include <QByteArray>

namespace QCoro::detail {


class QCoroIODevice {

    class OperationBase {
    public:
        Q_DISABLE_COPY(OperationBase)
        QCORO_DEFAULT_MOVE(OperationBase)

        virtual ~OperationBase() = default;

    protected:
        explicit OperationBase(QIODevice *device)
            : mDevice(device) {}

        virtual void finish(QCORO_STD::coroutine_handle<> awaitingCoroutine) {
            QObject::disconnect(mConn);
            QObject::disconnect(mCloseConn);
            // Delayed trigger
            QTimer::singleShot(0, [awaitingCoroutine]() mutable { awaitingCoroutine.resume(); });
        }

        QPointer<QIODevice> mDevice;
        QMetaObject::Connection mConn;
        QMetaObject::Connection mCloseConn;
        QMetaObject::Connection mFinishedConn;
    };

protected:
    class ReadOperation : public OperationBase {
    public:
        ReadOperation(QIODevice *device, std::function<QByteArray(QIODevice *)> &&resultCb)
            : OperationBase(device), mResultCb(std::move(resultCb)) {}

        Q_DISABLE_COPY(ReadOperation)
        QCORO_DEFAULT_MOVE(ReadOperation)

        virtual bool await_ready() const noexcept {
            return !mDevice
                    || !mDevice->isOpen()
                    || !mDevice->isReadable()
                    || mDevice->bytesAvailable() > 0;
        }

        virtual void await_suspend(QCORO_STD::coroutine_handle<> awaitingCoroutine) noexcept {
            Q_ASSERT(mDevice);
            mConn = QObject::connect(mDevice, &QIODevice::readyRead,
                                     std::bind(&ReadOperation::finish, this, awaitingCoroutine));
            mCloseConn = QObject::connect(mDevice, &QIODevice::aboutToClose,
                                     std::bind(&ReadOperation::finish, this, awaitingCoroutine));
        }

        auto await_resume() {
            return mResultCb(mDevice);
        }

    private:
        std::function<QByteArray(QIODevice *)> mResultCb;
    };

    class WriteOperation : public OperationBase {
    public:
        WriteOperation(QIODevice *device, const QByteArray &data)
            : OperationBase(device), mBytesToBeWritten(device->write(data)) {}

        Q_DISABLE_COPY(WriteOperation)
        QCORO_DEFAULT_MOVE(WriteOperation)

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
                                     [this, awaitingCoroutine](qint64 written) {
                                        mBytesWritten += written;
                                        if (mBytesWritten >= mBytesToBeWritten) {
                                            finish(awaitingCoroutine);
                                        }
                                    });
            mCloseConn = QObject::connect(mDevice, &QIODevice::aboutToClose,
                                    std::bind(&WriteOperation::finish, this, awaitingCoroutine));
        }

        qint64 await_resume() noexcept {
            return mBytesWritten;
        }

    private:
        qint64 mBytesToBeWritten = 0;
        qint64 mBytesWritten = 0;
    };

public:
    explicit QCoroIODevice(QIODevice *device)
        : mDevice{device}
    {}

    ReadOperation readAll() {
        return ReadOperation(mDevice, [](QIODevice *dev) { return dev->readAll(); });
    }

    ReadOperation read(qint64 maxSize) {
        return ReadOperation(mDevice, [maxSize](QIODevice *dev) { return dev->read(maxSize); });
    }

    ReadOperation readLine(qint64 maxSize = 0) {
        return ReadOperation(mDevice, [maxSize](QIODevice *dev) { return dev->readLine(maxSize); });
    }

    // TODO
    //auto bytesAvailable(qint64 minBytes) {

    auto write(const QByteArray &buffer) {
        return WriteOperation(mDevice, buffer);
    }

protected:
    QPointer<QIODevice> mDevice = {};
};



} /// namespace QCoro::detail
