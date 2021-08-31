// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "qcoro/task.h"

#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>

/*! \cond internal */

namespace QCoro::detail {

class DBusPendingCallAwaiter {
public:
    explicit DBusPendingCallAwaiter(const QDBusPendingCall &call);

    bool await_ready() const noexcept;
    void await_suspend(QCORO_STD::coroutine_handle<> awaitingCoroutine) noexcept;
    QDBusMessage await_resume() const;

private:
    QDBusPendingCall mCall;
};

template<typename T = void>
class DBusPendingReplyAwaiter {
public:
    explicit DBusPendingReplyAwaiter(const QDBusPendingReply<T> &reply) : mReply(reply) {}

    bool await_ready() const noexcept {
        return mReply.isFinished();
    }

    void await_suspend(QCORO_STD::coroutine_handle<> awaitingCoroutine) {
        auto *watcher = new QDBusPendingCallWatcher{mReply};
        QObject::connect(watcher, &QDBusPendingCallWatcher::finished,
                         [awaitingCoroutine](auto *watcher) mutable {
                             awaitingCoroutine.resume();
                             watcher->deleteLater();
                         });
    }

    QDBusPendingReply<T> await_resume() const {
        Q_ASSERT(mReply.isFinished());
        return mReply;
    }

private:
    QDBusPendingReply<T> mReply;
};

template<>
struct awaiter_type<QDBusPendingCall> {
    using type = DBusPendingCallAwaiter;
};

template<typename S>
struct awaiter_type<QDBusPendingReply<S>> {
    using type = DBusPendingReplyAwaiter<S>;
};

template<>
struct awaiter_type<QDBusPendingReply<>> {
    using type = DBusPendingReplyAwaiter<>;
};

} // namespace QCoro::detail

/*! \endcond */
