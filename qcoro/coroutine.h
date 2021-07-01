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

#include <concepts>

namespace QCoro {

//! A concept describing the Awaitable type
/*!
 * Awaitable type is a type that can be passed as an argument to co_await.
 */
template<typename T>
concept Awaitable = requires(T t) {
    { t.await_ready() }
    ->std::same_as<bool>;
    {t.await_suspend(std::declval<QCORO_STD::coroutine_handle<>>())};
    {t.await_resume()};
};

} // namespace QCoro

#endif
