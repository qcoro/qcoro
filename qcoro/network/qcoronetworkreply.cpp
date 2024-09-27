// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "qcoronetworkreply.h"
#include "qcoroiodevice_p.h"
#include "qcorosignal.h"

using namespace QCoro::detail;

namespace {

class ReplyWaitSignalHelper : public WaitSignalHelper {
    Q_OBJECT
public:
    ReplyWaitSignalHelper(const QNetworkReply *reply, void(QIODevice::*signal)())
        : WaitSignalHelper(reply, signal)
        , mError(connect(reply, &QNetworkReply::errorOccurred, this, [this]() { emitReady(false); }, Qt::QueuedConnection))
        , mFinished(connect(reply, &QNetworkReply::finished, this, [this]() { emitReady(true); }, Qt::QueuedConnection))
    {}
    ReplyWaitSignalHelper(const QNetworkReply *reply, void(QIODevice::*signal)(qint64))
        : WaitSignalHelper(reply, signal)
        , mError(connect(reply, &QNetworkReply::errorOccurred, this, [this]() { emitReady(0LL); }, Qt::QueuedConnection))
        , mFinished(connect(reply, &QNetworkReply::finished, this, [this]() { emitReady(0LL); }, Qt::QueuedConnection))
    {}

private:
    void cleanup() override {
        disconnect(mError);
        disconnect(mFinished);
        WaitSignalHelper::cleanup();
    }

    QMetaObject::Connection mError;
    QMetaObject::Connection mFinished;
};

} // namespace

struct QCoroNetworkReply::WaitForFinishedOperation::Private {
    static_assert(sizeof(std::unique_ptr<Private>)  + sizeof(void*) == sizeof(QPointer<QNetworkReply>),
                  "QCoroNetworkReply::WaitForFinishedOperation is not BC with previous version, see comment in header");

    Private(QPointer<QNetworkReply> reply)
        : reply(reply)
    {
        // Ensure the signal is emitted in the same thread as the reply, not on the thread
        // where the coroutine is resumed...
        if (reply) {
            dummy.moveToThread(reply->thread());
        }
    }

    QPointer<QNetworkReply> reply;
    QObject dummy;
};

QCoroNetworkReply::WaitForFinishedOperation::WaitForFinishedOperation(QPointer<QNetworkReply> reply)
    : d(std::make_unique<Private>(reply))
{
}

QCoroNetworkReply::WaitForFinishedOperation::~WaitForFinishedOperation() = default;

bool QCoroNetworkReply::WaitForFinishedOperation::await_ready() const noexcept {
    return !d->reply || d->reply->isFinished();
}

void QCoroNetworkReply::WaitForFinishedOperation::await_suspend(std::coroutine_handle<> awaitingCoroutine) {
    if (d->reply) {
        QObject::connect(d->reply, &QNetworkReply::finished, &d->dummy,
                         [awaitingCoroutine]() mutable { awaitingCoroutine.resume(); },
                         Qt::QueuedConnection);
    } else {
        awaitingCoroutine.resume();
    }
}

QNetworkReply *QCoroNetworkReply::WaitForFinishedOperation::await_resume() const noexcept {
    return d->reply;
}

QCoro::Task<std::optional<bool>> QCoroNetworkReply::waitForReadyReadImpl(std::chrono::milliseconds timeout) {
    const auto *reply = static_cast<QNetworkReply *>(mDevice.data());
    if (reply->isFinished()) {
        co_return true;
    }
    ReplyWaitSignalHelper helper(reply, &QNetworkReply::readyRead);
    co_return co_await qCoro(&helper, qOverload<bool>(&ReplyWaitSignalHelper::ready), timeout);
}

QCoro::Task<std::optional<qint64>> QCoroNetworkReply::waitForBytesWrittenImpl(std::chrono::milliseconds timeout) {
    const auto *reply = static_cast<QNetworkReply *>(mDevice.data());
    if (reply->isFinished()) {
        co_return false;
    }
    ReplyWaitSignalHelper helper(reply, &QNetworkReply::bytesWritten);
    co_return co_await qCoro(&helper, qOverload<qint64>(&ReplyWaitSignalHelper::ready), timeout);
}

QCoro::Task<bool> QCoroNetworkReply::waitForFinished(std::chrono::milliseconds timeout) {
    const auto *reply = static_cast<QNetworkReply *>(mDevice.data());
    if (reply->isFinished()) {
        co_return true;
    }

    const auto result = co_await qCoro(reply, &QNetworkReply::finished, timeout);
    co_return result.has_value();
}

#include "qcoronetworkreply.moc"