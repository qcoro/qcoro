// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "task.h"
#include "qcorocore_export.h"

#include <QMetaObject>
#include <QPointer>
#include <QTimer>

/*! \cond internal */

namespace QCoro::detail {

class QCOROCORE_EXPORT QCoroTimer {
private:
    class WaitForTimeoutOperation {
    public:
        explicit WaitForTimeoutOperation(QTimer *timer);
        explicit WaitForTimeoutOperation(QTimer &timer);

        bool await_ready() const noexcept;
        void await_suspend(std::coroutine_handle<> awaitingCoroutine);
        void await_resume() const;
    private:
        QMetaObject::Connection mConn;
        QPointer<QTimer> mTimer;
    };

    friend struct awaiter_type<QTimer *>;
    friend struct awaiter_type<QTimer>;

    QPointer<QTimer> mTimer;
public:
    explicit QCoroTimer(QTimer *timer);

    Task<void> waitForTimeout() const;
};

template<>
struct awaiter_type<QTimer *> {
    using type = QCoroTimer::WaitForTimeoutOperation;
};

template<>
struct awaiter_type<QTimer> {
    using type = QCoroTimer::WaitForTimeoutOperation;
};

} // namespace QCoro::detail

/*! \endcond */

//! Returns a coroutine-friendly wrapper for QTimer object.
/*!
 * Returns a wrapper for the QTimer \c timer that provides coroutine-friendly
 * way to co_await the timeout.
 *
 * @see docs/reference/qtimer.md
 */

inline auto qCoro(QTimer *timer) noexcept {
    return QCoro::detail::QCoroTimer{timer};
}

//! \copydoc qCoro(QTimer *)
inline auto qCoro(QTimer &timer) noexcept{
    return QCoro::detail::QCoroTimer{&timer};
}

