// SPDX-FileCopyrightText: 2023 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

/*
 * Do NOT include this file directly - include the QCoroTask header instead!
 */

#pragma once

#include "../qcorotask.h"

#include <QEventLoop>

#include <optional>

namespace QCoro
{

namespace detail
{

template <typename Awaitable>
requires TaskConvertible<Awaitable>
auto toTask(Awaitable &&future) -> QCoro::Task<detail::convertible_awaitable_return_type_t<Awaitable>> {
    co_return co_await future;
}

struct WaitContext {
    QEventLoop loop;
    bool coroutineFinished = false;
    std::exception_ptr exception;
};

//! Helper class to run a coroutine in a nested event loop.
/*!
 * We cannot just use QTimer or QMetaObject::invokeMethod() to schedule the func lambda to be
 * invoked from an event loop, because internally, Qt deallocates some structures when the
 * lambda returns, which causes invalid memory access and potentially double-free corruption
 * because the coroutine returns twice - once on suspend and once when it really finishes.
 * So instead we do basically what Qt does internally, but we make sure to not delete th
 * QFunctorSlotObjectWithNoArgs until after the event loop quits.
 */
template<Awaitable Awaitable>
Task<> runCoroutine(WaitContext &context, Awaitable &&awaitable) {
    try {
        co_await awaitable;
    } catch (...) {
        context.exception = std::current_exception();
    }
    context.coroutineFinished = true;
    context.loop.quit();
}

template<typename T, Awaitable Awaitable>
Task<> runCoroutine(WaitContext &context, std::optional<T> &result, Awaitable &&awaitable) {
    try {
        result.emplace(co_await awaitable);
    } catch (...) {
        context.exception = std::current_exception();
    }
    context.coroutineFinished = true;
    context.loop.quit();
}

template<typename T, Awaitable Awaitable>
T waitFor(Awaitable &&awaitable) {
    WaitContext context;
    if constexpr (std::is_void_v<T>) {
        runCoroutine(context, std::forward<Awaitable>(awaitable));
        if (!context.coroutineFinished) {
            context.loop.exec();
        }
        if (context.exception) {
            std::rethrow_exception(context.exception);
        }
    } else {
        std::optional<T> result;
        runCoroutine(context, result, std::forward<Awaitable>(awaitable));
        if (!context.coroutineFinished) {
            context.loop.exec();
        }
        if (context.exception) {
            std::rethrow_exception(context.exception);
        }
        return std::move(*result);
    }
}

} // namespace detail

template<typename T>
inline T waitFor(QCoro::Task<T> &task) {
    return detail::waitFor<T>(std::forward<QCoro::Task<T>>(task));
}

template<typename T>
inline T waitFor(QCoro::Task<T> &&task) {
    return detail::waitFor<T>(std::forward<QCoro::Task<T>>(task));
}

template<Awaitable Awaitable>
inline auto waitFor(Awaitable &&awaitable) {
    return detail::waitFor<detail::awaitable_return_type_t<Awaitable>>(std::forward<Awaitable>(awaitable));
}

} // namespace QCoro