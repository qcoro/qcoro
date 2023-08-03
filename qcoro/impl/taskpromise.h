// SPDX-FileCopyrightText: 2023 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

/*
 * Do NOT include this file directly - include the QCoroTask header instead!
 */

#pragma once

#include "../qcorotask.h"

#include <QtGlobal> // for Q_ASSERT

namespace QCoro::detail
{

template<typename T>
inline Task<T> TaskPromise<T>::get_return_object() noexcept {
    return Task<T>{std::coroutine_handle<TaskPromise>::from_promise(*this)};
}

template<typename T>
inline void TaskPromise<T>::unhandled_exception() {
    mValue = std::current_exception();
}

template<typename T>
inline void TaskPromise<T>::return_value(T &&value) noexcept {
    mValue.template emplace<T>(std::forward<T>(value));
}

template<typename T>
inline void TaskPromise<T>::return_value(const T &value) noexcept {
    mValue = value;
}

template<typename T>
template<typename U> requires QCoro::concepts::constructible_from<T, U>
inline void TaskPromise<T>::return_value(U &&value) noexcept {
    mValue = std::move(value);
}

template<typename T>
template<typename U> requires QCoro::concepts::constructible_from<T, U>
inline void TaskPromise<T>::return_value(const U &value) noexcept {
    mValue = value;
}

template<typename T>
inline T &TaskPromise<T>::result() & {
    if (std::holds_alternative<std::exception_ptr>(mValue)) {
        Q_ASSERT(std::get<std::exception_ptr>(mValue) != nullptr);
        std::rethrow_exception(std::get<std::exception_ptr>(mValue));
    }

    return std::get<T>(mValue);
}

template<typename T>
inline T &&TaskPromise<T>::result() && {
    if (std::holds_alternative<std::exception_ptr>(mValue)) {
        Q_ASSERT(std::get<std::exception_ptr>(mValue) != nullptr);
        std::rethrow_exception(std::get<std::exception_ptr>(mValue));
    }

    return std::move(std::get<T>(mValue));
}

inline Task<void> TaskPromise<void>::get_return_object() noexcept {
    return Task<void>{std::coroutine_handle<TaskPromise>::from_promise(*this)};
}

inline void TaskPromise<void>::unhandled_exception() {
    mException = std::current_exception();
}

inline void TaskPromise<void>::return_void() noexcept {}

inline void TaskPromise<void>::result() {
    if (mException) {
        std::rethrow_exception(mException);
    }
}

} // namespace QCoro::detail