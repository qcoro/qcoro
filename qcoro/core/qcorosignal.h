// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "coroutine.h"
#include "macros_p.h"
#include "concepts_p.h"

#include <QObject>
#include <QPointer>

#include <cassert>
#include <optional>

namespace QCoro::detail {

namespace concepts {

//! Simplistic QObject concept.
template<typename T>
concept QObject = requires(T *obj) {
    requires std::is_base_of_v<QObject, T>;
    requires std::is_same_v<decltype(T::staticMetaObject), const QMetaObject>;
};

} // namespace concepts

template<class>
struct args_tuple;

template<class R, class... Args>
struct args_tuple<R(Args...)> {
    using types = std::tuple<Args...>;
};

template<class R, class T, class... Args>
struct args_tuple<R (T::*)(Args...)> {
    using types = std::tuple<Args...>;
};

template<concepts::QObject T, typename FuncPtr>
class QCoroSignal {
    using ArgsTuple = typename args_tuple<FuncPtr>::types;

public:
    QCoroSignal(T *obj, FuncPtr &&funcPtr) : mObj(obj), mFuncPtr(std::forward<FuncPtr>(funcPtr)) {}

    bool await_ready() const noexcept {
        return mObj.isNull();
    }

    void await_suspend(std::coroutine_handle<> awaitingCoroutine) noexcept {
        mConn = QObject::connect(
            mObj, mFuncPtr, mObj,
            [this, awaitingCoroutine](auto &&...args) mutable {
                QObject::disconnect(mConn);

                mResult.emplace(std::forward<decltype(args)>(args)...);
                awaitingCoroutine.resume();
            },
            Qt::QueuedConnection);
    }

    auto await_resume() {
        // TODO: Ignore QPrivateSignal...
        if constexpr (std::tuple_size_v<ArgsTuple> == 1) {
            assert(mResult.has_value());
            return std::move(std::get<0>(mResult.value()));
        } else if constexpr (std::tuple_size_v<ArgsTuple> > 0) {
            assert(mResult.has_value());
            return std::move(mResult.value());
        }
    }

private:
    QPointer<T> mObj;
    FuncPtr mFuncPtr;
    QMetaObject::Connection mConn;
    std::optional<ArgsTuple> mResult;
};

template<concepts::QObject T, typename FuncPtr>
QCoroSignal(T *, FuncPtr &&) -> QCoroSignal<T, FuncPtr>;

} // namespace QCoro::detail

//! Allows co_awaiting on signal emission.
/*!
 * Returns an Awaitable object that allows co_awaiting for a signal to
 * be emitted. The result of the co_awaiting is a tuple with the signal
 * arguments.
 *
 * @see docs/reference/coro.md
 */
template<QCoro::detail::concepts::QObject T, typename FuncPtr>
inline auto qCoro(T *obj, FuncPtr &&ptr) {
    return QCoro::detail::QCoroSignal(obj, std::forward<FuncPtr>(ptr));
}

