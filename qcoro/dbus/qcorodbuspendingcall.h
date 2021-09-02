// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "task.h"

#include <QDBusPendingCallWatcher>

class QDBusMessage;
class QDBusPendingCall;

/*! \cond internal */

namespace QCoro::detail {

class QCoroDBusPendingCall {
private:
    class WaitForFinishedOperation {
    public:
        explicit WaitForFinishedOperation(const QDBusPendingCall &call);

        bool await_ready() const noexcept;
        void await_suspend(QCORO_STD::coroutine_handle<> awaitingCoroutine) noexcept;
        QDBusMessage await_resume() const;
    private:
        const QDBusPendingCall &mCall;
    };

    const QDBusPendingCall &mCall;

    friend struct awaiter_type<QDBusPendingCall>;
public:
    explicit QCoroDBusPendingCall(const QDBusPendingCall &call);

    WaitForFinishedOperation waitForFinished();
};

template<>
struct awaiter_type<QDBusPendingCall> {
    using type = QCoroDBusPendingCall::WaitForFinishedOperation;
};

} // namespace QCoro::detail

inline auto qCoro(const QDBusPendingCall &call) {
    return QCoro::detail::QCoroDBusPendingCall{call};
}

/*! \endcond */
