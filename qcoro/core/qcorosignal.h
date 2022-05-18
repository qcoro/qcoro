// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "coroutine.h"
#include "macros_p.h"
#include "concepts_p.h"
#include "task.h"
#include "qcoroasyncgenerator.h"

#include <QObject>
#include <QPointer>
#include <QTimer>

#include <cassert>
#include <optional>
#include <deque>

namespace QCoro::detail {

namespace concepts {

//! Simplistic QObject concept.
template<typename T>
concept QObject = requires(T *obj) {
    requires std::is_base_of_v<::QObject, T>;
    requires std::is_same_v<decltype(T::staticMetaObject), const QMetaObject>;
};

} // namespace concepts

template<class>
struct args_tuple;

template<class R, class... Args>
struct args_tuple<R(Args...)> {
    using type = std::tuple<std::remove_cvref_t<Args>...>;
};

template<class R, class T, class... Args>
struct args_tuple<R (T::*)(Args...)> {
    using type = std::tuple<std::remove_cvref_t<Args>...>;
};

template<class R, class T, class Arg>
struct args_tuple<R (T::*)(Arg)> {
    using type = std::remove_cvref_t<Arg>;
};

template<class R, class Arg>
struct args_tuple<R(Arg)> {
    using type = std::remove_cvref_t<Arg>;
};

template<concepts::QObject T, typename FuncPtr>
class QCoroSignalBase {
public:
    // TODO: Ignore QPrivateSignal
    using result_type = std::optional<typename args_tuple<std::remove_cvref_t<FuncPtr>>::type>;

    QCoroSignalBase(const QCoroSignalBase &) = delete;
    QCoroSignalBase &operator=(const QCoroSignalBase &) = delete;
    QCoroSignalBase(QCoroSignalBase &&) noexcept = default;
    QCoroSignalBase &operator=(QCoroSignalBase &&) noexcept = default;

    ~QCoroSignalBase() {
        if (mConn) {
            QObject::disconnect(mConn);
        }
    }

protected:
    QCoroSignalBase(T *obj, FuncPtr &&funcPtr, std::chrono::milliseconds timeout)
        : mObj(obj), mFuncPtr(std::forward<FuncPtr>(funcPtr)) {
        if (timeout.count() > -1) {
            mTimeoutTimer = std::make_unique<QTimer>();
            mTimeoutTimer->setInterval(timeout);
            mTimeoutTimer->setSingleShot(true);
        }
    }

    void handleTimeout(std::coroutine_handle<> awaitingCoroutine) {
        if (mTimeoutTimer) {
            QObject::connect(mTimeoutTimer.get(), &QTimer::timeout, mObj,
                [this, awaitingCoroutine]() {
                    QObject::disconnect(mConn);
                    awaitingCoroutine.resume();
                });
            mTimeoutTimer->start();
        }
    }

protected:
    QPointer<T> mObj;
    FuncPtr mFuncPtr;
    QMetaObject::Connection mConn;
    std::unique_ptr<QTimer> mTimeoutTimer;
};

template<concepts::QObject T, typename FuncPtr>
class QCoroSignal : public QCoroSignalBase<T, FuncPtr> {
public:
    using typename QCoroSignalBase<T, FuncPtr>::result_type;

    QCoroSignal(T *obj, FuncPtr &&ptr, std::chrono::milliseconds timeout)
        : QCoroSignalBase<T, FuncPtr>(obj, std::forward<FuncPtr>(ptr), timeout) {}

    QCoroSignal(QCoroSignal &&other) noexcept
        : QCoroSignalBase<T, FuncPtr>(std::move(other))
        , mResult(std::move(other.mResult)) {
        QObject::disconnect(this->mConn);
        setupConnection();
    }

    QCoroSignal &operator=(QCoroSignal &&other) noexcept {
        QCoroSignalBase<T, FuncPtr>::operator=(std::move(other));
        std::swap(mResult, other.mResult);
        QObject::disconnect(this->mConn);
        setupConnection();
        return *this;
    }

    bool await_ready() const noexcept {
        return this->mObj.isNull();
    }

    void await_suspend(std::coroutine_handle<> awaitingCoroutine) noexcept {
        this->handleTimeout(awaitingCoroutine);
        mAwaitingCoroutine = awaitingCoroutine;
        setupConnection();
    }

    result_type await_resume() {
        return std::move(mResult);
    }

private:
    void setupConnection() {
        this->mConn = QObject::connect(
            this->mObj, this->mFuncPtr, this->mObj,
            [this](auto &&...args) mutable {
                if (this->mTimeoutTimer) {
                    this->mTimeoutTimer->stop();
                }
                QObject::disconnect(this->mConn);

                mResult.emplace(std::forward<decltype(args)>(args)...);
                if (mAwaitingCoroutine) {
                    mAwaitingCoroutine.resume();
                }
            },
            Qt::QueuedConnection);
    }

    result_type mResult;
    std::coroutine_handle<> mAwaitingCoroutine;
};

template<concepts::QObject T, typename FuncPtr>
QCoroSignal(T *, FuncPtr &&, std::chrono::milliseconds) -> QCoroSignal<T, FuncPtr>;

template<concepts::QObject T, typename FuncPtr>
class QCoroSignalQueue : public QCoroSignalBase<T, FuncPtr> {
public:
    using typename QCoroSignalBase<T, FuncPtr>::result_type;

    QCoroSignalQueue(T *obj, FuncPtr &&ptr, std::chrono::milliseconds timeout)
        : QCoroSignalBase<T, FuncPtr>(obj, std::forward<FuncPtr>(ptr), timeout) {
        setupConnection();
    }

    QCoroSignalQueue(QCoroSignalQueue &&other) noexcept
        : QCoroSignalBase<T, FuncPtr>(std::move(other))
        , mAwaitingCoroutine(std::move(other.mAwaitingCoroutine))
        , mQueue(std::move(other.mQueue)) {
        QObject::disconnect(this->mConn);
        setupConnection();
    }

    QCoroSignalQueue &operator=(QCoroSignalQueue &&other) noexcept {
        QCoroSignalBase<T, FuncPtr>::operator=(std::move(other));
        std::swap(mAwaitingCoroutine, other.mAwaitingCoroutine);
        std::swap(mQueue, other.mQueue);
        QObject::disconnect(this->mConn);
        setupConnection();
    }

    bool await_ready() const noexcept {
        return !mQueue.empty() || this->mObj.isNull();
    }

    void await_suspend(std::coroutine_handle<> awaitingCoroutine) noexcept {
        this->handleTimeout(awaitingCoroutine);
        mAwaitingCoroutine = awaitingCoroutine;
    }

    result_type await_resume() {
        if (mQueue.empty()) {
            return std::nullopt;
        }

        auto result = std::move(mQueue.front());
        mQueue.pop_front();
        return result;
    }

private:
    void setupConnection() {
        this->mConn = QObject::connect(
            this->mObj, this->mFuncPtr, this->mObj,
            [this](auto && ...args) mutable {
                if (this->mTimeoutTimer) {
                    this->mTimeoutTimer->stop();
                }
                mQueue.emplace_back(std::forward<decltype(args)>(args) ...);

                if (mAwaitingCoroutine) {
                    mAwaitingCoroutine.resume();
                }
            }, Qt::QueuedConnection);

    }

    std::coroutine_handle<> mAwaitingCoroutine;
    std::deque<typename result_type::value_type> mQueue;
};

template<concepts::QObject T, typename FuncPtr>
QCoroSignalQueue(T *, FuncPtr &&, std::chrono::milliseconds) -> QCoroSignalQueue<T, FuncPtr>;


} // namespace QCoro::detail

//! Allows co_awaiting on signal emission with a timeout.
/*!
 * Returns an Awaitable object that allows co_awaiting for a singal
 * to be emitted within the specified timeout. If the signal has exactly
 * one argument, then the value of the argument is returned as a result
 * of awaiting the coroutine. If the signal has two or more arguments,
 * then the arguments are returned as a tuple. If the signal has no
 * arguments, then the result of the coroutine is an empty tuple.
 *
 * If the timeout occurs before the signal is emitted, the result of the
 * coroutine is an empty optional. If the \c timeout is -1 the operation
 * will never time out.
 */
template<QCoro::detail::concepts::QObject T, typename FuncPtr>
inline auto qCoro(T *obj, FuncPtr &&ptr, std::chrono::milliseconds timeout)
    -> QCoro::Task<typename QCoro::detail::QCoroSignal<T, FuncPtr>::result_type> {
    auto result = co_await QCoro::detail::QCoroSignal(obj, std::forward<FuncPtr>(ptr), timeout);
    co_return std::move(result);
}

//! Allows co_awaiting on signal emission.
/*!
 * Returns an Awaitable object that allows co_awaiting for a signal to
 * be emitted. If the signal has exactly one argument, then the value
 * of the argument is returned as a result of awaiting the coroutine.
 * If the signal has two or more arguments, then the arguments are
 * returned as a tuple. If the signal has no arguments, then the result
 * of the coroutine is an empty tuple.
 *
 * @see docs/reference/coro.md
 */
template<QCoro::detail::concepts::QObject T, typename FuncPtr>
inline auto qCoro(T *obj, FuncPtr &&ptr)
    -> QCoro::Task<typename QCoro::detail::QCoroSignal<T, FuncPtr>::result_type::value_type> {
    auto result = co_await qCoro<T, FuncPtr>(obj, std::forward<FuncPtr>(ptr), std::chrono::milliseconds{-1});
    co_return std::move(*result);
}

template<QCoro::detail::concepts::QObject T, typename FuncPtr>
inline auto qCoroSignalGenerator(T *obj, FuncPtr &&ptr,
                                 std::chrono::milliseconds timeout = std::chrono::milliseconds{-1})
    -> QCoro::AsyncGenerator<typename QCoro::detail::QCoroSignalQueue<T, FuncPtr>::result_type::value_type> {

    constexpr auto innerGenerator = [](auto signalQueue) ->
        QCoro::AsyncGenerator<typename QCoro::detail::QCoroSignalQueue<T, FuncPtr>::result_type::value_type> {
        Q_FOREVER {
            auto result = co_await signalQueue;
            if (!result.has_value()) { // timeout
                break;
            }

            co_yield std::move(*result);
        }
    };

    QCoro::detail::QCoroSignalQueue signalQueue{obj, std::forward<FuncPtr>(ptr), timeout};
    return innerGenerator(std::move(signalQueue));
}
