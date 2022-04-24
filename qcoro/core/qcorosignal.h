// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "coroutine.h"
#include "macros_p.h"
#include "concepts_p.h"
#include "task.h"

#include <QObject>
#include <QPointer>
#include <QTimer>

#include <cassert>
#include <optional>

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
    using type = std::tuple<Args...>;
};

template<class R, class T, class... Args>
struct args_tuple<R (T::*)(Args...)> {
    using type = std::tuple<Args...>;
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
class QCoroSignal {
public:
    // TODO: Ignore QtPrivateSignal
    using result_type = std::optional<typename args_tuple<FuncPtr>::type>;

    QCoroSignal(T *obj, FuncPtr &&funcPtr, std::chrono::milliseconds timeout)
        : mObj(obj), mFuncPtr(std::forward<FuncPtr>(funcPtr)) {
        if (timeout.count() > -1) {
            mTimeoutTimer = std::make_unique<QTimer>();
            mTimeoutTimer->setInterval(timeout);
            mTimeoutTimer->setSingleShot(true);
        }
    }

    bool await_ready() const noexcept {
        return mObj.isNull();
    }

    void await_suspend(std::coroutine_handle<> awaitingCoroutine) noexcept {
        if (mTimeoutTimer) {
            QObject::connect(mTimeoutTimer.get(), &QTimer::timeout, mObj,
                [this, awaitingCoroutine]() {
                    QObject::disconnect(mConn);
                    awaitingCoroutine.resume();
                });
            mTimeoutTimer->start();
        }
        mConn = QObject::connect(
            mObj, mFuncPtr, mObj,
            [this, awaitingCoroutine](auto &&...args) mutable {
                if (mTimeoutTimer) {
                    mTimeoutTimer->stop();
                }
                QObject::disconnect(mConn);

                mResult.emplace(std::forward<decltype(args)>(args)...);
                awaitingCoroutine.resume();
            },
            Qt::QueuedConnection);
    }

    result_type await_resume() {
        return std::move(mResult);
    }

private:
    QPointer<T> mObj;
    FuncPtr mFuncPtr;
    QMetaObject::Connection mConn;
    std::unique_ptr<QTimer> mTimeoutTimer;
    result_type mResult;
};

template<concepts::QObject T, typename FuncPtr>
QCoroSignal(T *, FuncPtr &&, std::chrono::milliseconds) -> QCoroSignal<T, FuncPtr>;

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
    QCoro::detail::QCoroSignal coroSignal(obj, std::forward<FuncPtr>(ptr), timeout);
    auto result = co_await coroSignal;
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

