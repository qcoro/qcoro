// SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "../coroutine.h"

#include <atomic>

namespace QCoro::detail {

class TaskFinalSuspend {
public:
    //! Constructs the awaitable, passing it a handle to the co_awaiting coroutine
    /*!
     * \param[in] awaitingCoroutine handle of the coroutine that is co_awaiting the current
     * coroutine (continuation).
     */
    explicit TaskFinalSuspend(std::coroutine_handle<> awaitingCoroutine)
        : mAwaitingCoroutine(awaitingCoroutine) {}

    //! Returns whether the just finishing coroutine should do final suspend or not
    /*!
     * If the coroutine is not being co_awaited by another coroutine, then don't
     * suspend and let the code reach the end of the coroutine, which will take
     * care of cleaning everything up. Otherwise we suspend and let the awaiting
     * coroutine to clean up for us.
     */
    bool await_ready() const noexcept {
        return false;
    }

    //! Called by the compiler when the just-finished coroutine is suspended. for the very last time.
    /*!
     * It is given handle of the coroutine that is being co_awaited (that is the current
     * coroutine). If there is a co_awaiting coroutine and it has not been resumed yet,
     * it resumes it.
     *
     * Finally, it destroys the just finished coroutine and frees all allocated resources.
     *
     * \param[in] finishedCoroutine handle of the just finished coroutine
     */
    template<typename Promise>
    void await_suspend(std::coroutine_handle<Promise> finishedCoroutine) noexcept {
        auto &promise = finishedCoroutine.promise();

        if (promise.mResumeAwaiter.exchange(true, std::memory_order_acq_rel)) {
            promise.mAwaitingCoroutine.resume();
        }

        // The handle will be destroyed here only if the associated Task has already been destroyed
        if (promise.setDestroyHandle()) {
            finishedCoroutine.destroy();
        }
    }

    //! Called by the compiler when the just-finished coroutine should be resumed.
    /*!
     * In reality this should never be called, as our coroutine is done, so it won't be resumed.
     * In any case, this method does nothing.
     * */
    constexpr void await_resume() const noexcept {}

private:
    //! Handle of the coroutine co_awaiting the current coroutine.
    std::coroutine_handle<> mAwaitingCoroutine;
};

} // namespace QCoro::detail