// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "task.h"

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

template<typename T1 = void, typename T2 = void, typename T3 = void, typename T4 = void,
         typename T5 = void, typename T6 = void, typename T7 = void, typename T8 = void>
class DBusPendingReplyAwaiter {
public:
    explicit DBusPendingReplyAwaiter(const QDBusPendingReply<T1, T2, T3, T4, T5, T6, T7, T8> &reply) : mReply(reply) {}

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

    QDBusPendingReply<T1, T2, T3, T4, T5, T6, T7, T8> await_resume() const {
        Q_ASSERT(mReply.isFinished());
        return mReply;
    }

private:
    QDBusPendingReply<T1, T2, T3, T4, T5, T6, T7, T8> mReply;
};

template<>
struct awaiter_type<QDBusPendingCall> {
    using type = DBusPendingCallAwaiter;
};

template<>
struct awaiter_type<QDBusPendingReply<>> {
    using type = DBusPendingReplyAwaiter<>;
};

template<typename S>
struct awaiter_type<QDBusPendingReply<S>> {
    using type = DBusPendingReplyAwaiter<S>;
};

template<typename S1, typename S2>
struct awaiter_type<QDBusPendingReply<S1, S2>> {
    using type = DBusPendingReplyAwaiter<S1, S2>;
};

template<typename S1, typename S2, typename S3>
struct awaiter_type<QDBusPendingReply<S1, S2, S3>> {
    using type = DBusPendingReplyAwaiter<S1, S2, S3>;
};

template<typename S1, typename S2, typename S3, typename S4>
struct awaiter_type<QDBusPendingReply<S1, S2, S3, S4>> {
    using type = DBusPendingReplyAwaiter<S1, S2, S3, S4>;
};

template<typename S1, typename S2, typename S3, typename S4,
         typename S5>
struct awaiter_type<QDBusPendingReply<S1, S2, S3, S4, S5>> {
    using type = DBusPendingReplyAwaiter<S1, S2, S3, S4, S5>;
};

template<typename S1, typename S2, typename S3, typename S4,
         typename S5, typename S6>
struct awaiter_type<QDBusPendingReply<S1, S2, S3, S4, S5, S6>> {
    using type = DBusPendingReplyAwaiter<S1, S2, S3, S4, S5, S6>;
};

template<typename S1, typename S2, typename S3, typename S4,
         typename S5, typename S6, typename S7>
struct awaiter_type<QDBusPendingReply<S1, S2, S3, S4, S5, S6, S7>> {
    using type = DBusPendingReplyAwaiter<S1, S2, S3, S4, S5, S6, S7>;
};

template<typename S1, typename S2, typename S3, typename S4,
         typename S5, typename S6, typename S7, typename S8>
struct awaiter_type<QDBusPendingReply<S1, S2, S3, S4, S5, S6, S7, S8>> {
    using type = DBusPendingReplyAwaiter<S1, S2, S3, S4, S5, S6, S7, S8>;
};

} // namespace QCoro::detail

/*! \endcond */
