// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "qcorodbuspendingcall.h"
#include "qcorosignal.h"

#include <QDBusPendingCall>

using namespace QCoro::detail;

QCoroDBusPendingCall::WaitForFinishedOperation::WaitForFinishedOperation(const QDBusPendingCall &call)
    : mCall(call)
{}

bool QCoroDBusPendingCall::WaitForFinishedOperation::await_ready() const noexcept {
    return mCall.isFinished();
}

void QCoroDBusPendingCall::WaitForFinishedOperation::await_suspend(std::coroutine_handle<> awaitingCoroutine) noexcept {
auto *watcher = new QDBusPendingCallWatcher{mCall};
    QObject::connect(watcher, &QDBusPendingCallWatcher::finished,
                     [awaitingCoroutine](auto *watcher) mutable {
                         awaitingCoroutine.resume();
                         watcher->deleteLater();
                     });
}

QDBusMessage QCoroDBusPendingCall::WaitForFinishedOperation::await_resume() const {
    Q_ASSERT(mCall.isFinished());
    return mCall.reply();
}

QCoroDBusPendingCall::QCoroDBusPendingCall(const QDBusPendingCall &call)
    : mCall(call)
{}

QCoro::Task<QDBusMessage> QCoroDBusPendingCall::waitForFinished() {
    QDBusPendingCallWatcher watcher{mCall};
    co_await qCoro(&watcher, &QDBusPendingCallWatcher::finished);
    co_return watcher.reply();
}

