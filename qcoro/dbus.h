// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "task.h"

#include <QDBusPendingCallWatcher>
#include <QDBusPendingCall>
#include <QDBusMessage>

/*! \cond internal */

namespace QCoro::detail {

class DBusPendingCallAwaiter {
public:
    explicit DBusPendingCallAwaiter(const QDBusPendingCall &call)
        : mCall(call)
    {}

    bool await_ready() const noexcept {
        return mCall.isFinished();
    }

    void await_suspend(QCORO_STD::coroutine_handle<> awaitingCoroutine) {
        auto *watcher = new QDBusPendingCallWatcher{mCall};
        QObject::connect(watcher, &QDBusPendingCallWatcher::finished,
                         [awaitingCoroutine](auto *watcher) mutable {
                            awaitingCoroutine.resume();
                            watcher->deleteLater();
                        });
    }

    QDBusMessage await_resume() const {
        Q_ASSERT(mCall.isFinished());
        return mCall.reply();
    }

private:
    QDBusPendingCall mCall;
};

template<>
struct awaiter_type<QDBusPendingCall> {
    using type = DBusPendingCallAwaiter;
};

} // namespace QCoro::detail

/*! \endcond */
