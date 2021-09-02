// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "task.h"

#include <type_traits>

#include <QFuture>
#include <QFutureWatcher>

/*! \cond internal */

namespace QCoro::detail {

template<typename T>
class QCoroFuture final {
private:
    template<typename T_>
    class WaitForFinishedOperationBase {
    public:
        WaitForFinishedOperationBase(const QFuture<T_> &future) {
            QObject::connect(&mFutureWatcher, &QFutureWatcher<T_>::finished,
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

    private:
        void futureReady() {
            mAwaitingCoroutine.resume();
        }

        QCORO_STD::coroutine_handle<> mAwaitingCoroutine;

    protected:
        QFutureWatcher<T_> mFutureWatcher;
    };

    class WaitForFinishedOperationImplT : public WaitForFinishedOperationBase<T> {
    public:
        using WaitForFinishedOperationBase<T>::WaitForFinishedOperationBase;

        T await_resume() const noexcept {
            return this->mFutureWatcher.result();
        }
    };

    class WaitForFinishedOperationImplVoid : public WaitForFinishedOperationBase<void> {
    public:
        using WaitForFinishedOperationBase<void>::WaitForFinishedOperationBase;

        void await_resume() const noexcept {}
    };

    using WaitForFinishedOperation = std::conditional_t<
        std::is_void_v<T>, WaitForFinishedOperationImplVoid, WaitForFinishedOperationImplT>;

    friend struct awaiter_type<QFuture<T>>;

    QFuture<T> mFuture;

public:
    explicit QCoroFuture(const QFuture<T> &future)
        : mFuture(future) {}

    WaitForFinishedOperation waitForFinished() {
        return WaitForFinishedOperation{mFuture};
    }

};

template<typename T>
struct awaiter_type<QFuture<T>> {
    using type = typename QCoroFuture<T>::WaitForFinishedOperation;
};

} // namespace QCoro::detail

/*! \endcond */

//! Returns a coroutine-friendly wrapper for QFuture object.
/*!
 * Returns a wrapper for the QFuture \c f that provides coroutine-friendly
 * way to co_await the completion of the future.
 *
 * @see docs/reference/qfuture.md
 */
template<typename T>
inline auto qCoro(const QFuture<T> &f) noexcept {
    return QCoro::detail::QCoroFuture<T>{f};
}

