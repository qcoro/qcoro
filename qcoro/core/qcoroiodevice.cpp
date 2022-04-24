// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "qcoroiodevice.h"
#include "qcoroiodevice_p.h"
#include "qcorosignal.h"

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

QCoroIODevice::ReadAllOperation::ReadAllOperation(QIODevice *device)
    : ReadOperation(device, [](QIODevice *d) { return d->readAll(); }) {}

QCoroIODevice::ReadAllOperation::ReadAllOperation(QIODevice &device)
    : ReadAllOperation(&device) {}

QCoroIODevice::QCoroIODevice(QIODevice *device)
    : mDevice{device}
{}

QCoro::Task<QByteArray> QCoroIODevice::readAll(std::chrono::milliseconds timeout) {
    const auto device = mDevice;
    if (!co_await waitForReadyRead(timeout)) {
        co_return QByteArray{};
    }

    co_return device->readAll();
}

QCoro::Task<QByteArray> QCoroIODevice::read(qint64 maxSize, std::chrono::milliseconds timeout) {
    const auto device = mDevice;
    if (!co_await waitForReadyRead(timeout)) {
        co_return QByteArray{};
    }

    co_return device->read(maxSize);
}

QCoro::Task<QByteArray> QCoroIODevice::readLine(qint64 maxSize, std::chrono::milliseconds timeout) {
    const auto device = mDevice;
    if (!co_await waitForReadyRead(timeout)) {
        co_return QByteArray{};
    }

    co_return device->readLine(maxSize);
}

QCoro::Task<qint64> QCoroIODevice::write(const QByteArray &buffer) {
    auto bytesWritten = mDevice->write(buffer);
    while (bytesWritten > 0) {
        const auto flushed = co_await waitForBytesWritten(-1);
        bytesWritten -= flushed.value();
    }

    co_return bytesWritten;
}

QCoro::Task<bool> QCoroIODevice::waitForReadyRead(int timeout_msecs) {
    return waitForReadyRead(std::chrono::milliseconds(timeout_msecs));
}

QCoro::Task<bool> QCoroIODevice::waitForReadyRead(std::chrono::milliseconds timeout) {
    if (!mDevice->isReadable()) {
        co_return false;
    }
    if (mDevice->bytesAvailable() > 0) {
        co_return true;
    }

    const auto result = co_await waitForReadyReadImpl(timeout);
    co_return result.has_value();
}

QCoro::Task<std::optional<qint64>> QCoroIODevice::waitForBytesWritten(int timeout_msecs) {
    return waitForBytesWritten(std::chrono::milliseconds(timeout_msecs));
}

QCoro::Task<std::optional<qint64>> QCoroIODevice::waitForBytesWritten(std::chrono::milliseconds timeout) {
    if (!mDevice->isWritable()) {
        co_return std::nullopt;
    }
    if (mDevice->bytesToWrite() == 0) {
        co_return 0;
    }

    const auto result = co_await waitForBytesWrittenImpl(timeout);
    co_return result;
}

QCoro::Task<std::optional<bool>> QCoroIODevice::waitForReadyReadImpl(std::chrono::milliseconds timeout) {
    WaitSignalHelper helper(mDevice.data(), &QIODevice::readyRead);
    co_return co_await qCoro(&helper, qOverload<bool>(&WaitSignalHelper::ready), timeout);
}

QCoro::Task<std::optional<qint64>> QCoroIODevice::waitForBytesWrittenImpl(std::chrono::milliseconds timeout) {
    WaitSignalHelper helper(mDevice.data(), &QIODevice::bytesWritten);
    co_return co_await qCoro(&helper, qOverload<qint64>(&WaitSignalHelper::ready), timeout);
}