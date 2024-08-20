// SPDX-FileCopyrightText: 2023 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

/*
 * Do NOT include this file directly - include the QCoroTask header instead!
 */

#pragma once

#include "../qcorotask.h"
#include "bits/features.h"

namespace QCoro::detail
{

inline TaskPromiseBase::TaskPromiseBase()
    : mRefCount(1)
{
}

inline std::suspend_never TaskPromiseBase::initial_suspend() const noexcept {
    return {};
}

inline auto TaskPromiseBase::final_suspend() const noexcept {
    return TaskFinalSuspend{mAwaitingCoroutines};
}

template<typename T, typename Awaiter>
inline auto TaskPromiseBase::await_transform(T &&value) {
    return Awaiter{std::forward<T>(value)};
}

template<Awaitable T>
inline auto && TaskPromiseBase::await_transform(T &&awaitable) {
    return std::forward<T>(awaitable);
}

template<Awaitable T>
inline auto &TaskPromiseBase::await_transform(T &awaitable) {
    return awaitable;
}

template<typename T>
inline void TaskPromiseBase::addAwaitingCoroutine(std::coroutine_handle<T> awaitingCoroutine) {
    if constexpr (std::is_same_v<T, TaskPromiseBase>) {
        mAwaitingCoroutines.emplace_back(CoroutineHandle{std::in_place_index<0>, awaitingCoroutine});
    } else {
        mAwaitingCoroutines.emplace_back(CoroutineHandle{std::in_place_index<1>, awaitingCoroutine});
    }
}

inline bool TaskPromiseBase::hasAwaitingCoroutine() const {
    return !mAwaitingCoroutines.empty();
}

inline void TaskPromiseBase::derefCoroutine() {
    if (--mRefCount == 0) {
        destroyCoroutine();
    }
}

inline void TaskPromiseBase::refCoroutine() {
    ++mRefCount;
}

inline void TaskPromiseBase::destroyCoroutine(bool wakeUpAwaiters) {
    if (wakeUpAwaiters) {
        for (auto &awaiter : mAwaitingCoroutines) {
            if (const auto qcoro_handle = std::get_if<std::coroutine_handle<TaskPromiseBase>>(&awaiter); qcoro_handle != nullptr) {
                qcoro_handle->resume();
            } else {
                std::get<std::coroutine_handle<>>(awaiter).resume();
            }
        }
    }

    mRefCount = 0;
    auto handle = std::coroutine_handle<TaskPromiseBase>::from_promise(*this);
    handle.destroy();

}

} // namespace QCoro::detail
