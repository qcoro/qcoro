// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "task.h"

#include <QFuture>
#include <QFutureWatcher>

/*! \cond internal */

namespace QCoro::detail {

template<typename T>
class FutureAwaiterBase {
public:
    explicit FutureAwaiterBase(QFuture<T> future) {
        QObject::connect(&mFutureWatcher, &QFutureWatcher<T>::finished,
                         [this]() { futureReady(); });
        mFutureWatcher.setFuture(future);
    }

    bool await_ready() const noexcept {
        return mFutureWatcher.isFinished() || mFutureWatcher.isCanceled();
    }

    void await_suspend(QCORO_STD::coroutine_handle<> awaitingCoroutine) {
        if (mFutureWatcher.isFinished()) {
            awaitingCoroutine.resume();
        }

        mAwaitingCoroutine = awaitingCoroutine;
    }

protected:
    void futureReady() {
        mAwaitingCoroutine.resume();
    }

    QCORO_STD::coroutine_handle<> mAwaitingCoroutine;
    QFutureWatcher<T> mFutureWatcher;
};

template<typename T>
class FutureAwaiter : public FutureAwaiterBase<T> {
public:
    using FutureAwaiterBase<T>::FutureAwaiterBase;

    T await_resume() const {
        return this->mFutureWatcher.result();
    }
};

template<>
class FutureAwaiter<void> : public FutureAwaiterBase<void> {
public:
    using FutureAwaiterBase<void>::FutureAwaiterBase;

    void await_resume() const {}
};

template<typename T>
struct awaiter_type<QFuture<T>> {
    using type = FutureAwaiter<T>;
};

} // namespace QCoro::detail

/*! \endcond */
