// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once
#if defined(__clang__)
#include <experimental/coroutine>
#define QCORO_STD std::experimental
#elif defined(__GNUC__) || defined(_MSC_VER)
#include <coroutine>
#define QCORO_STD std
#else
#pragma error "Current compiler doesn't support Coroutines."
#endif

// Moc doesn't seem to understand something in the <concepts> header...
#ifndef Q_MOC_RUN

#include "concepts_p.h"

namespace QCoro {

namespace detail {

template<typename T>
concept has_await_methods = requires(T t) {
    { t.await_ready() } -> std::same_as<bool>;
    {t.await_suspend(std::declval<QCORO_STD::coroutine_handle<>>())};
    {t.await_resume()};
};

template<typename T>
concept has_operator_coawait = requires(T t) {
    // TODO: Check that result of co_await() satisfies Awaitable again
    { t.operator co_await() };
};


} // namespace detail

//! A concept describing the Awaitable type
/*!
 * Awaitable type is a type that can be passed as an argument to co_await.
 */
template<typename T>
concept Awaitable = detail::has_operator_coawait<T> ||
                    detail::has_await_methods<T>;

} // namespace QCoro

#endif
