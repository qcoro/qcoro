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
        #if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        , mError(connect(reply, &QNetworkReply::errorOccurred, this, [this]() { emitReady(false); }))
        #endif
        , mFinished(connect(reply, &QNetworkReply::finished, this, [this]() { emitReady(true); }))
    {}
    ReplyWaitSignalHelper(const QNetworkReply *reply, void(QIODevice::*signal)(qint64))
        : WaitSignalHelper(reply, signal)
        #if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        , mError(connect(reply, &QNetworkReply::errorOccurred, this, [this]() { emitReady(0LL); }))
        #endif
        , mFinished(connect(reply, &QNetworkReply::finished, this, [this]() { emitReady(0LL); }))
    {}

private:
    void cleanup() override {
        #if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        disconnect(mError);
        #endif
        disconnect(mFinished);
        WaitSignalHelper::cleanup();
    }

    #if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    QMetaObject::Connection mError;
    #endif
    QMetaObject::Connection mFinished;
};

} // namespace

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