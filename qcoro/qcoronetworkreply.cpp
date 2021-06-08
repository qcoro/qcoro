// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "qcoronetworkreply.h"

using namespace QCoro::detail;

bool QCoroNetworkReply::ReadOperation::await_ready() const noexcept {
    return QCoroIODevice::ReadOperation::await_ready() ||
           static_cast<const QNetworkReply *>(mDevice.data())->isFinished();
}

void QCoroNetworkReply::ReadOperation::await_suspend(QCORO_STD::coroutine_handle<> awaitingCoroutine) noexcept {
    QCoroIODevice::ReadOperation::await_suspend(awaitingCoroutine);

    mFinishedConn = QObject::connect(
        static_cast<QNetworkReply *>(mDevice.data()), &QNetworkReply::finished,
        std::bind(&ReadOperation::finish, this, awaitingCoroutine));
}

void QCoroNetworkReply::ReadOperation::finish(QCORO_STD::coroutine_handle<> awaitingCoroutine) {
    QObject::disconnect(mFinishedConn);
    QCoroIODevice::ReadOperation::finish(awaitingCoroutine);
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
