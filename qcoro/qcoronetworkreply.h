// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "qcoroiodevice.h"

#include <QNetworkReply>

namespace QCoro::detail {

class QCoroNetworkReply final : private QCoroIODevice {
private:
    class ReadOperation final : public QCoroIODevice::ReadOperation {
    public:
        using QCoroIODevice::ReadOperation::ReadOperation;

        bool await_ready() const noexcept final {
            return QCoroIODevice::ReadOperation::await_ready() ||
                   static_cast<const QNetworkReply *>(mDevice.data())->isFinished();
        }

        void await_suspend(QCORO_STD::coroutine_handle<> awaitingCoroutine) noexcept final {
            QCoroIODevice::ReadOperation::await_suspend(awaitingCoroutine);

            mFinishedConn = QObject::connect(
                static_cast<QNetworkReply *>(mDevice.data()), &QNetworkReply::finished,
                std::bind(&ReadOperation::finish, this, awaitingCoroutine));
        }

    private:
        void finish(QCORO_STD::coroutine_handle<> awaitingCoroutine) final {
            QObject::disconnect(mFinishedConn);
            QCoroIODevice::ReadOperation::finish(awaitingCoroutine);
        }

        QMetaObject::Connection mFinishedConn;
    };

public:
    using QCoroIODevice::QCoroIODevice;

    ReadOperation readAll() {
        return ReadOperation(mDevice, [](QIODevice *dev) { return dev->readAll(); });
    }

    ReadOperation read(qint64 maxSize) {
        return ReadOperation(mDevice, [maxSize](QIODevice *dev) { return dev->read(maxSize); });
    }

    ReadOperation readLine(qint64 maxSize = 0) {
        return ReadOperation(mDevice, [maxSize](QIODevice *dev) { return dev->readLine(maxSize); });
    }
};

} // namespace QCoro::detail
