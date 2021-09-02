// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "task.h"
#include "macros_p.h"

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
        explicit WaitForFinishedOperationBase(const QFuture<T_> &future)
                : mFuture(future) {}
        Q_DISABLE_COPY(WaitForFinishedOperationBase)
        QCORO_DEFAULT_MOVE(WaitForFinishedOperationBase)

        bool await_ready() const noexcept {
            return mFuture.isFinished() || mFuture.isCanceled();
        }

        void await_suspend(QCORO_STD::coroutine_handle<> awaitingCoroutine) {
            auto *watcher = new QFutureWatcher<T_>();
            auto cb = [watcher, awaitingCoroutine]() mutable {
                watcher->deleteLater();
                awaitingCoroutine.resume();
            };
            QObject::connect(watcher, &QFutureWatcher<T_>::finished, cb);
            QObject::connect(watcher, &QFutureWatcher<T_>::canceled, cb);
            watcher->setFuture(mFuture);
        }

    protected:
        QFuture<T_> mFuture;
    };

    class WaitForFinishedOperationImplT : public WaitForFinishedOperationBase<T> {
    public:
        using WaitForFinishedOperationBase<T>::WaitForFinishedOperationBase;

        T await_resume() const noexcept {
            return this->mFuture.result();
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

