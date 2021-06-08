// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "dbus.h"

using namespace QCoro::detail;

DBusPendingCallAwaiter::DBusPendingCallAwaiter(const QDBusPendingCall &call)
    : mCall(call)
{}

bool DBusPendingCallAwaiter::await_ready() const noexcept {
    return mCall.isFinished();
}

void DBusPendingCallAwaiter::await_suspend(QCORO_STD::coroutine_handle<> awaitingCoroutine) noexcept {
    auto *watcher = new QDBusPendingCallWatcher{mCall};
    QObject::connect(watcher, &QDBusPendingCallWatcher::finished,
                     [awaitingCoroutine](auto *watcher) mutable {
                         awaitingCoroutine.resume();
                         watcher->deleteLater();
                     });
}

QDBusMessage DBusPendingCallAwaiter::await_resume() const {
    Q_ASSERT(mCall.isFinished());
    return mCall.reply();
}

