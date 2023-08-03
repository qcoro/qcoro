// SPDX-FileCopyrightText: 2023 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

/*
 * Do NOT include this file directly - include the QCoroTask header instead!
 */

#pragma once

#include "../qcorotask.h"

#include <optional>

namespace QCoro
{

template<typename T>
inline Task<T>::Task(std::coroutine_handle<promise_type> coroutine) : mCoroutine(coroutine) {}

template<typename T>
inline Task<T>::Task(Task &&other) noexcept : mCoroutine(other.mCoroutine) {
    other.mCoroutine = nullptr;
}

    //! The task can be move-assigned.
template<typename T>
inline auto Task<T>::operator=(Task &&other) noexcept -> Task & {
    if (std::addressof(other) != this) {
        if (mCoroutine) {
            // The coroutine handle will be destroyed only after TaskFinalSuspend
            if (mCoroutine.promise().setDestroyHandle()) {
                mCoroutine.destroy();
            }
        }

        mCoroutine = other.mCoroutine;
        other.mCoroutine = nullptr;
    }
    return *this;
}

template<typename T>
inline Task<T>::~Task() {
    if (!mCoroutine) return;

    // The coroutine handle will be destroyed only after TaskFinalSuspend
    if (mCoroutine.promise().setDestroyHandle()) {
        mCoroutine.destroy();
    }
}

template<typename T>
inline bool Task<T>::isReady() const {
    return !mCoroutine || mCoroutine.done();
}

template<typename T>
inline auto Task<T>::operator co_await() const noexcept {
    //! Specialization of the TaskAwaiterBase that returns the promise result by value
    class TaskAwaiter : public detail::TaskAwaiterBase<promise_type> {
    public:
        TaskAwaiter(std::coroutine_handle<promise_type> awaitedCoroutine)
            : detail::TaskAwaiterBase<promise_type>{awaitedCoroutine} {}

        //! Called when the co_awaited coroutine is resumed.
        /*
            * \return the result from the coroutine's promise, factically the
            * value co_returned by the coroutine. */
        auto await_resume() {
            Q_ASSERT(this->mAwaitedCoroutine != nullptr);
            if constexpr (!std::is_void_v<T>) {
                return std::move(this->mAwaitedCoroutine.promise().result());
            } else {
                // Wil re-throw exception, if any is stored
                this->mAwaitedCoroutine.promise().result();
            }
        }
    };

    return TaskAwaiter{mCoroutine};
}

template<typename T>
template<typename ThenCallback>
requires (std::is_invocable_v<ThenCallback> || (!std::is_void_v<T> && std::is_invocable_v<ThenCallback, T>))
inline auto Task<T>::then(ThenCallback &&callback) {
    // Provide a custom error handler that simply re-throws the current exception
    return thenImpl(std::forward<ThenCallback>(callback), [](const auto &) { throw; });
}

template<typename T>
template<typename ThenCallback, typename ErrorCallback>
requires ((std::is_invocable_v<ThenCallback> || (!std::is_void_v<T> && std::is_invocable_v<ThenCallback, T>)) &&
            std::is_invocable_v<ErrorCallback, const std::exception &>)
inline auto Task<T>::then(ThenCallback &&callback, ErrorCallback &&errorCallback) {
    return thenImpl(std::forward<ThenCallback>(callback), std::forward<ErrorCallback>(errorCallback));
}

template<typename T>
template<typename ThenCallback, typename ... Args>
inline auto Task<T>::invokeCb(ThenCallback &&callback, [[maybe_unused]] Args && ... args) {
    if constexpr (std::is_invocable_v<ThenCallback, Args ...>) {
        return callback(std::forward<Args>(args) ...);
    } else {
        return callback();
    }
}

template<typename T>
template<typename R, typename ErrorCallback, typename U>
inline auto Task<T>::handleException(ErrorCallback &errCb, const std::exception &exception) -> U {
    errCb(exception);
    if constexpr (!std::is_void_v<U>) {
        return U{};
    }
}

template<typename T>
template<typename ThenCallback, typename ErrorCallback, typename R>
inline auto Task<T>::thenImpl(ThenCallback &&thenCallback, ErrorCallback &&errorCallback) -> std::conditional_t<detail::isTask_v<R>, R, Task<R>> {
    auto thenCb = std::forward<ThenCallback>(thenCallback);
    auto errCb = std::forward<ErrorCallback>(errorCallback);
    if constexpr (std::is_void_v<value_type>) {
        try {
            co_await *this;
        } catch (const std::exception &e) {
            co_return handleException<R>(errCb, e);
        }
        if constexpr (detail::isTask_v<R>) {
            co_return co_await invokeCb(thenCb);
        } else {
            co_return invokeCb(thenCb);
        }
    } else {
        std::optional<T> value;
        try {
            value = co_await *this;
        } catch (const std::exception &e) {
            co_return handleException<R>(errCb, e);
        }
        if constexpr (detail::isTask_v<R>) {
            co_return co_await invokeCb(thenCb, std::move(*value));
        } else {
            co_return invokeCb(thenCb, std::move(*value));
        }
    }
}

} // namespace QCoro