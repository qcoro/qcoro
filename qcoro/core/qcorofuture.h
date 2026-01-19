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
            auto done = std::make_shared<std::atomic<bool>>(false);
            auto finished = std::make_shared<QMetaObject::Connection>();
            auto canceled = std::make_shared<QMetaObject::Connection>();

            auto resume = [watcher, awaitingCoroutine, done, finished, canceled]() mutable {
                bool expected = false;
                if (!done->compare_exchange_strong(expected, true)) {
                    return;
                }
                QObject::disconnect(*finished);
                QObject::disconnect(*canceled);
                watcher->deleteLater();
                awaitingCoroutine.resume();
            };

            *finished = QObject::connect(watcher, &QFutureWatcherBase::finished, watcher, resume);
            *canceled = QObject::connect(watcher, &QFutureWatcherBase::canceled, watcher, resume);

            watcher->setFuture(mFuture);
        }

    protected:
        QFuture<T_> mFuture;
    };

    class WaitForFinishedOperationImplT : public WaitForFinishedOperationBase<T> {
    public:
        using WaitForFinishedOperationBase<T>::WaitForFinishedOperationBase;

        T await_resume() {
            if (this->mFuture.isFinished()) {
                this->mFuture.waitForFinished();

                if (!this->mFuture.isResultReadyAt(0)) {
                    throw std::runtime_error("QFuture finished without a result");
                }

                return this->mFuture.result();
            }

            if constexpr (std::is_default_constructible_v<T>) {
                return {};
            }
            else {
                // Future was invalid or canceled without finishing. For types without
                // default constructors, we can't return {}, so we throw an exception.
                throw std::runtime_error("QFuture was invalid or canceled");
            }
        }
    };

    class WaitForFinishedOperationImplVoid : public WaitForFinishedOperationBase<void> {
    public:
        using WaitForFinishedOperationBase<void>::WaitForFinishedOperationBase;

        void await_resume() {
            // This won't block, since we know for sure that the QFuture is already finished.
            // The weird side-effect of this function is that it will re-throw the stored
            // exception.
            if (this->mFuture.isFinished()) {
                this->mFuture.waitForFinished();
            }
        }
    };

    using WaitForFinishedOperation = std::conditional_t<
        std::is_void_v<T>, WaitForFinishedOperationImplVoid, WaitForFinishedOperationImplT>;

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    template<typename T_ =  T> requires (!std::is_void_v<T_>)
    class TakeResultOperation : public WaitForFinishedOperationBase<T_> {
    public:
        using WaitForFinishedOperationBase<T>::WaitForFinishedOperationBase;

        T_ await_resume() {
            if (this->mFuture.isFinished()) {
                this->mFuture.waitForFinished();

                if (!this->mFuture.isResultReadyAt(0)) {
                    throw std::runtime_error("QFuture finished without a result");
                }

                return this->mFuture.takeResult();
            }

            if constexpr (std::is_default_constructible_v<T>) {
                return {};
            }
            else {
                // Future was invalid or canceled without finishing. For types without
                // default constructors, we can't return {}, so we throw an exception.
                throw std::runtime_error("QFuture was invalid or canceled");
            }
        }
    };

#endif


    friend struct awaiter_type<QFuture<T>>;

    QFuture<T> mFuture;

public:
    explicit QCoroFuture(const QFuture<T> &future)
        : mFuture(future) {}

    /*!
     * \brief Equivalent to using `QCoroFuture::result()`.
     *
     * This function is provided for backwards API compatibility, new code should use
     * `QCoroFuture::result()` instead.
     *
     * \see QCoroFuture<T>::result()
     */
    Task<T> waitForFinished() {
        co_return co_await WaitForFinishedOperation{mFuture};
    }

    /*!
     * \brief Asynchronously waits for the future to finish and returns the result.
     *
     * This is equivalent to using `QFutureWatcher` to wait for the future to finish and
     * then obtainign the result using `QFuture::result()`.
     *
     * \see QCoroFuture<T>::takeResult()
     */
    Task<T> result() {
        co_return co_await WaitForFinishedOperation{mFuture};
    }

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    /*!
     * \brief Asynchronously waits for the future to finish and takes (moves) the result from the future object.
     *
     * This is useful when you want to move the result from the future object into a local variable without
     * copying it or when working with move-only types.
     *
     * Using this is equivalent to using `QFutureWatcher` to wait for the future to finish and then
     * obtaining the result using `QFuture::takeResult()`.
     *
     * \see QCoroFuture<T>::result()
     */
    Task<T> takeResult() requires (!std::is_void_v<T>) {
        co_return std::move(co_await TakeResultOperation<T>{mFuture});
    }
#endif
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
