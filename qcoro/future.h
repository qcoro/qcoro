// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "task.h"

#include <QFuture>
#include <QFutureWatcher>

namespace QCoro::detail
{

template<typename T>
class FutureAwaiter {
public:
    explicit FutureAwaiter(QFuture<T> future)
    {
        QObject::connect(&mFutureWatcher, &QFutureWatcher<T>::finished,
                [this]() { futureReady(); });
        mFutureWatcher.setFuture(future);
    }

    bool await_ready() const noexcept {
        return mFutureWatcher.isFinished();
    }

    void await_suspend(QCORO_STD::coroutine_handle<> awaitingCoroutine) {
        if (mFutureWatcher.isFinished()) {
            awaitingCoroutine.resume();
        }

        mAwaitingCoroutine = awaitingCoroutine;
    }

    T await_resume() const {
        return mFutureWatcher.result();
    }

private:
    void futureReady() {
        mAwaitingCoroutine.resume();
    }

    QCORO_STD::coroutine_handle<> mAwaitingCoroutine;
    QFutureWatcher<T> mFutureWatcher;
};

template<typename T>
struct awaiter_type<QFuture<T>> {
    using type = FutureAwaiter<T>;
};

} // namespace QCoro::detail
