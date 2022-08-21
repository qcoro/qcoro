// SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "../coroutine.h"
#include "taskfinalsuspend.h"

namespace QCoro {
template<typename T>
class CancellableTask;
} // namespace QCoro

namespace QCoro::detail {

class PromiseBase {
public:
    std::suspend_never initial_suspend() const noexcept {
        return {};
    }

    auto final_suspend() const noexcept {
        return TaskFinalSuspend{mAwaitingCoroutine};
    }

    bool setAwaitingCoroutine(std::coroutine_handle<> awaitingCoroutine) {
        mAwaitingCoroutine = awaitingCoroutine;
        return !mResumeAwaiter.exchange(true, std::memory_order_acq_rel);
    }

    bool hasAwaitingCoroutine() const {
        return static_cast<bool>(mAwaitingCoroutine);
    }

    bool setDestroyHandle() noexcept {
        return mDestroyHandle.exchange(true, std::memory_order_acq_rel);
    }

private:
    friend class TaskFinalSuspend;
    template<typename T> friend class QCoro::CancellableTask;

    std::coroutine_handle<> mAwaitingCoroutine;
    std::atomic<bool> mResumeAwaiter{false};
    std::atomic<bool> mDestroyHandle{false};
};

} // namespace QCoro::detail
