// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "qcoronetworkreply.h"

using namespace QCoro::detail;

bool QCoroNetworkReply::ReadOperation::await_ready() const noexcept {
    return QCoroIODevice::ReadOperation::await_ready() ||
           static_cast<const QNetworkReply *>(mDevice.data())->isFinished();
}

void QCoroNetworkReply::ReadOperation::await_suspend(std::coroutine_handle<> awaitingCoroutine) noexcept {
    QCoroIODevice::ReadOperation::await_suspend(awaitingCoroutine);

    mFinishedConn = QObject::connect(
        static_cast<QNetworkReply *>(mDevice.data()), &QNetworkReply::finished,
        std::bind(&ReadOperation::finish, this, awaitingCoroutine));
}

void QCoroNetworkReply::ReadOperation::finish(std::coroutine_handle<> awaitingCoroutine) {
    QObject::disconnect(mFinishedConn);
    QCoroIODevice::ReadOperation::finish(awaitingCoroutine);
}

QCoroNetworkReply::WaitForFinishedOperation::WaitForFinishedOperation(QPointer<QNetworkReply> reply)
    : mReply(reply)
{}

bool QCoroNetworkReply::WaitForFinishedOperation::await_ready() const noexcept {
    return !mReply || mReply->isFinished();
}

void QCoroNetworkReply::WaitForFinishedOperation::await_suspend(std::coroutine_handle<> awaitingCoroutine) {
    if (mReply) {
        QObject::connect(mReply, &QNetworkReply::finished,
                         [awaitingCoroutine]() mutable { awaitingCoroutine.resume(); });
    } else {
        awaitingCoroutine.resume();
    }
}

QNetworkReply *QCoroNetworkReply::WaitForFinishedOperation::await_resume() const noexcept {
    return mReply;
}

QCoroNetworkReply::ReadOperation QCoroNetworkReply::readAll() {
    return ReadOperation(mDevice, [](QIODevice *dev) { return dev->readAll(); });
}

QCoroNetworkReply::ReadOperation QCoroNetworkReply::read(qint64 maxSize) {
    return ReadOperation(mDevice, [maxSize](QIODevice *dev) { return dev->read(maxSize); });
}

QCoroNetworkReply::ReadOperation QCoroNetworkReply::readLine(qint64 maxSize) {
    return ReadOperation(mDevice, [maxSize](QIODevice *dev) { return dev->readLine(maxSize); });
}

QCoroNetworkReply::WaitForFinishedOperation QCoroNetworkReply::waitForFinished() {
    return WaitForFinishedOperation(static_cast<QNetworkReply *>(mDevice.data()));
}
