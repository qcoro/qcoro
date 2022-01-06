// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "macros_p.h"
#include "coroutine.h"

#include <QTimer>
#include <memory>

namespace QCoro::detail {

//! Base class for co_awaitable waitFor* operations.
template<typename T>
class WaitOperationBase {
public:
    Q_DISABLE_COPY(WaitOperationBase)
    QCORO_DEFAULT_MOVE(WaitOperationBase)

    ~WaitOperationBase() = default;

    bool await_resume() noexcept {
        return !mTimedOut;
    }

protected:
    WaitOperationBase(T *obj, int timeout_msecs) : mObj{obj} {
        if (timeout_msecs > -1) {
            mTimeoutTimer = std::make_unique<QTimer>();
            mTimeoutTimer->setInterval(timeout_msecs);
            mTimeoutTimer->setSingleShot(true);
        }
    }

    void startTimeoutTimer(std::coroutine_handle<> awaitingCoroutine) {
        if (!mTimeoutTimer) {
            return;
        }

        QObject::connect(mTimeoutTimer.get(), &QTimer::timeout,
                         [this, awaitingCoroutine]() mutable {
                             mTimedOut = true;
                             resume(awaitingCoroutine);
                         });
        mTimeoutTimer->start();
    }

    void resume(std::coroutine_handle<> awaitingCoroutine) {
        if (mTimeoutTimer) {
            mTimeoutTimer->stop();
        }

        QObject::disconnect(mConn);

        QTimer::singleShot(0, [awaitingCoroutine]() mutable { awaitingCoroutine.resume(); });
    }

    QPointer<T> mObj;
    std::unique_ptr<QTimer> mTimeoutTimer;
    QMetaObject::Connection mConn;
    bool mTimedOut = false;
};

} // namespace QCoro::detail
