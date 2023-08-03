// SPDX-FileCopyrightText: 2023 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

/*
 * Do NOT include this file directly - include the QCoroTask header instead!
 */

#pragma once

#include "../qcorotask.h"

namespace QCoro::detail
{

template<typename Promise>
inline bool TaskAwaiterBase<Promise>::await_ready() const noexcept {
    return !mAwaitedCoroutine || mAwaitedCoroutine.done();
}

template<typename Promise>
inline void TaskAwaiterBase<Promise>::await_suspend(std::coroutine_handle<> awaitingCoroutine) noexcept {
    mAwaitedCoroutine.promise().addAwaitingCoroutine(awaitingCoroutine);
}

template<typename Promise>
inline TaskAwaiterBase<Promise>::TaskAwaiterBase(std::coroutine_handle<Promise> awaitedCoroutine)
    : mAwaitedCoroutine(awaitedCoroutine) {}

} // namespace QCoro::detail