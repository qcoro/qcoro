// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "qcorotask.h"
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

        void await_suspend(std::coroutine_handle<> awaitingCoroutine) {
            auto *watcher = new QFutureWatcher<T_>();
            QObject::connect(watcher, &QFutureWatcherBase::finished, [watcher, awaitingCoroutine]() mutable {
                watcher->deleteLater();
                awaitingCoroutine.resume();
            });
            watcher->setFuture(mFuture);
        }

    protected:
        QFuture<T_> mFuture;
    };

    class WaitForFinishedOperationImplT : public WaitForFinishedOperationBase<T> {
    public:
        using WaitForFinishedOperationBase<T>::WaitForFinishedOperationBase;

        T await_resume() const {
            return this->mFuture.result();
        }
    };

    class WaitForFinishedOperationImplVoid : public WaitForFinishedOperationBase<void> {
    public:
        using WaitForFinishedOperationBase<void>::WaitForFinishedOperationBase;

        void await_resume() {
            // This won't block, since we know for sure that the QFuture is already finished.
            // The weird side-effect of this function is that it will re-throw the stored
            // exception.
            this->mFuture.waitForFinished();
        }
    };

    using WaitForFinishedOperation = std::conditional_t<
        std::is_void_v<T>, WaitForFinishedOperationImplVoid, WaitForFinishedOperationImplT>;

    friend struct awaiter_type<QFuture<T>>;

    QFuture<T> mFuture;

public:
    explicit QCoroFuture(const QFuture<T> &future)
        : mFuture(future) {}

    /*!
     \brief Operation that allows co_awaiting completion of the running future.

     <!-- doc-waitForFinished-start -->
     Waits until the future is finished and then returns the result of the future (or nothing, if the future
     is a `QFuture<void>`.

     If the call is already finished or has an error, the coroutine will not suspend and the `co_await`
     expression will return immediatelly.

     This is a coroutine-friendly equivalent to using [`QFutureWatcher`][qdoc-qfuturewatcher]:

     ```cpp
     QFuture<QString> future = QtConcurrent::run([]() { ... });
     QFutureWatcher<QString> *watcher = new QFutureWatcher<QString>();
     QObject::connect(watcher, &QFutureWatcher<QString>::finished,
                      this, [watcher]() {
                          watcher->deleteLater();
                          const QStrign result = watcher->result();
                          ...
                      });
     ```

     You can also await completion of the future without using `QCoroFuture` at all by directly co-awaiting the
     `QFuture` object:

     ```cpp
     const QString result = co_await QtConcurrent::run([]() { ... });
     ```

     <!-- doc-waitForFinished-end -->

     [qfuturewatcher]: https://doc.qt.io/qt-5/qfuturewatcher.html
    */
    Task<T> waitForFinished() {
        co_return co_await WaitForFinishedOperation{mFuture};
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
