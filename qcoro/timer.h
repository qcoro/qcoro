// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "task.h"

#include <QPointer>
#include <QTimer>
#include <QMetaObject>

#include <QDebug>

namespace QCoro::detail
{

class TimerAwaiter {
public:
    explicit TimerAwaiter(QTimer &timer)
        : mTimer(&timer)
    {}
    explicit TimerAwaiter(QTimer *timer)
        : mTimer(timer)
    {}

    bool await_ready() const noexcept {
        return !mTimer || !mTimer->isActive();
    }

    void await_suspend(QCORO_STD::coroutine_handle<> awaitingCoroutine) {
        if (mTimer && mTimer->isActive()) {
            mConn = QObject::connect(mTimer, &QTimer::timeout,
                    [this, awaitingCoroutine]() mutable {
                        QObject::disconnect(mConn);
                        awaitingCoroutine.resume();
                    });
        } else {
            awaitingCoroutine.resume();
        }
    }

    void await_resume() const {}

private:
    QMetaObject::Connection mConn;
    QPointer<QTimer> mTimer;
};

template<>
struct awaiter_type<QTimer *> {
    using type = TimerAwaiter;
};
template<>
struct awaiter_type<QTimer> {
    using type = TimerAwaiter;
};

} // namespace QCoro::detail
