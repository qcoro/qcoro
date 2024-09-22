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

inline TaskFinalSuspend::TaskFinalSuspend(const std::vector<std::coroutine_handle<>> &awaitingCoroutines)
    : mAwaitingCoroutines(awaitingCoroutines) {}

inline bool TaskFinalSuspend::await_ready() const noexcept {
    return false;
}

template<typename Promise>
inline void TaskFinalSuspend::await_suspend(std::coroutine_handle<Promise> finishedCoroutine) noexcept {
    auto &promise = finishedCoroutine.promise();

    for (auto &awaiter : mAwaitingCoroutines) {
        awaiter.resume();
    }
    mAwaitingCoroutines.clear();

    // The handle will be destroyed here only if the associated Task has already been destroyed
    promise.derefCoroutine();
}

constexpr void TaskFinalSuspend::await_resume() const noexcept {}

} // namespace QCoro::detail
