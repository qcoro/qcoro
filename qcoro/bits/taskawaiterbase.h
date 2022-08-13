// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "../coroutine.h"

namespace QCoro::detail {

//! Base-class for Awaiter objects returned by the \c Task<T> operator co_await().
template<typename Promise>
class TaskAwaiterBase {
public:
    //! Returns whether to co_await
    bool await_ready() const noexcept {
        return !mAwaitedCoroutine || mAwaitedCoroutine.done();
    }

    //! Called by co_await in a coroutine that co_awaits our awaited coroutine managed by the current task.
    /*!
     * In other words, let's have a case like this:
     * \code{.cpp}
     * Task<> doSomething() {
     *    ...
     *    co_return result;
     * };
     *
     * Task<> getSomething() {
     *    ...
     *    const auto something = co_await doSomething();
     *    ...
     * }
     * \endcode
     *
     * If this Awaiter object is an awaiter of the doSomething() coroutine (e.g. has been constructed
     * by the co_await), then \c mAwaitedCoroutine is the handle of the doSomething() coroutine,
     * and \c awaitingCoroutine is a handle of the getSomething() coroutine which is awaiting the
     * completion of the doSomething() coroutine.
     *
     * This is implemented by passing the awaiting coroutine handle to the promise of the
     * awaited coroutine. When the awaited coroutine finishes, the promise will take care of
     * resuming the awaiting coroutine. At the same time this function resumes the awaited
     * coroutine.
     *
     * \param[in] awaitingCoroutine handle of the coroutine that is currently co_awaiting the
     * coroutine represented by this Tak.
     * \return returns whether the awaiting coroutine should be suspended, or whether the
     * co_awaited coroutine has finished synchronously and the co_awaiting coroutine doesn't
     * have to suspend.
     */
    bool await_suspend(std::coroutine_handle<> awaitingCoroutine) noexcept {
        return mAwaitedCoroutine.promise().setAwaitingCoroutine(awaitingCoroutine);
    }

protected:
    //! Constucts a new Awaiter object.
    /*!
     * \param[in] coroutine hande for the coroutine that is being co_awaited.
     */
    explicit TaskAwaiterBase(std::coroutine_handle<Promise> awaitedCoroutine)
        : mAwaitedCoroutine(awaitedCoroutine) {}

    //! Handle of the coroutine that is being co_awaited by this awaiter
    std::coroutine_handle<Promise> mAwaitedCoroutine = {};
};

} // namespace QCoro::detail
