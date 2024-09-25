// SPDX-FileCopyrightText: 2023 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

/*
 * Do NOT include this file directly - include the QCoroTask header instead!
 */

#pragma once

#include "../qcorotask.h"

#include <optional>
#include <QDebug>

namespace QCoro::detail {

template<typename T, template<typename> class TaskImpl, typename PromiseType>
inline TaskBase<T, TaskImpl, PromiseType>::TaskBase(std::coroutine_handle<PromiseType> coroutine) : mCoroutine(coroutine) {
    mCoroutine.promise().refCoroutine();
}

template<typename T, template<typename> class TaskImpl, typename PromiseType>
inline TaskBase<T, TaskImpl, PromiseType>::TaskBase(TaskBase &&other) noexcept : mCoroutine(other.mCoroutine) {
    other.mCoroutine = nullptr;
}

//! The task can be move-assigned.
template<typename T, template<typename> class TaskImpl, typename PromiseType>
inline auto TaskBase<T, TaskImpl, PromiseType>::operator=(TaskBase &&other) noexcept -> TaskBase & {
    if (std::addressof(other) != this) {
        if (mCoroutine) {
            mCoroutine.promise().derefCoroutine();
        }

        mCoroutine = other.mCoroutine;
        other.mCoroutine = nullptr;
    }
    return *this;
}

template<typename T, template<typename> class TaskImpl, typename PromiseType>
inline TaskBase<T, TaskImpl, PromiseType>::~TaskBase() {
    if (mCoroutine) {
        mCoroutine.promise().derefCoroutine();
    }
}

template<typename T, template<typename> class TaskImpl, typename PromiseType>
inline bool TaskBase<T, TaskImpl, PromiseType>::isReady() const {
    return !mCoroutine || mCoroutine.done();
}

template<typename T, template<typename> class TaskImpl, typename PromiseType>
inline auto TaskBase<T, TaskImpl, PromiseType>::operator co_await() const noexcept {
    //! Specialization of the TaskAwaiterBase that returns the promise result by value
    class TaskAwaiter : public detail::TaskAwaiterBase<PromiseType> {
    public:
        TaskAwaiter(std::coroutine_handle<PromiseType> awaitedCoroutine)
            : detail::TaskAwaiterBase<PromiseType>{awaitedCoroutine} {}

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

    return TaskAwaiter{this->mCoroutine};
}

template<typename T, template<typename> class TaskImpl, typename PromiseType>
template<typename ThenCallback>
requires (std::is_invocable_v<ThenCallback> || (!std::is_void_v<T> && std::is_invocable_v<ThenCallback, T>))
inline auto TaskBase<T, TaskImpl, PromiseType>::then(ThenCallback &&callback) & {
    // Provide a custom error handler that simply re-throws the current exception
    return thenImplRef(*this, std::forward<ThenCallback>(callback), [](const auto &) { throw; });
}

template<typename T, template<typename> class TaskImpl, typename PromiseType>
template<typename ThenCallback>
requires (std::is_invocable_v<ThenCallback> || (!std::is_void_v<T> && std::is_invocable_v<ThenCallback, T>))
inline auto TaskBase<T, TaskImpl, PromiseType>::then(ThenCallback &&callback) && {
    // When chaining LazyTask continuation to a temporary (xvalue) LazyTask, the temporary LazyTask must be captured in
    // thenImpl() by value as (as an argument), since thenImpl() is suspended immediately (it's a lazy task) and the temporary
    // would be destroyed before the thenImpl() is resumed and has a chance to consume/co_await the temporary LazyTask.
    return thenImpl<TaskBase>(std::move(*this), std::forward<ThenCallback>(callback), [](const auto &) { throw; });
}

template<typename T, template<typename> class TaskImpl, typename PromiseType>
template<typename ThenCallback, typename ErrorCallback>
requires ((std::is_invocable_v<ThenCallback> || (!std::is_void_v<T> && std::is_invocable_v<ThenCallback, T>)) &&
            std::is_invocable_v<ErrorCallback, const std::exception &>)
inline auto TaskBase<T, TaskImpl, PromiseType>::then(ThenCallback &&callback, ErrorCallback &&errorCallback) & {
    return thenImplRef(*this, std::forward<ThenCallback>(callback), std::forward<ErrorCallback>(errorCallback));
}

template<typename T, template<typename> class TaskImpl, typename PromiseType>
template<typename ThenCallback, typename ErrorCallback>
requires ((std::is_invocable_v<ThenCallback> || (!std::is_void_v<T> && std::is_invocable_v<ThenCallback, T>)) &&
            std::is_invocable_v<ErrorCallback, const std::exception &>)
inline auto TaskBase<T, TaskImpl, PromiseType>::then(ThenCallback &&callback, ErrorCallback &&errorCallback) && {
    return thenImpl<TaskBase>(std::move(*this), std::forward<ThenCallback>(callback), std::forward<ErrorCallback>(errorCallback));
}

template<typename T, template<typename> class TaskImpl, typename PromiseType>
template<typename ThenCallback, typename ... Args>
inline auto TaskBase<T, TaskImpl, PromiseType>::invokeCb(ThenCallback &&callback, [[maybe_unused]] Args && ... args) {
    if constexpr (std::is_invocable_v<ThenCallback, Args ...>) {
        return callback(std::forward<Args>(args) ...);
    } else {
        return callback();
    }
}

template<typename T, template<typename> class TaskImpl, typename PromiseType>
template<typename R, typename ErrorCallback, typename U>
inline auto TaskBase<T, TaskImpl, PromiseType>::handleException(ErrorCallback &errCb, const std::exception &exception) -> U {
    errCb(exception);
    if constexpr (!std::is_void_v<U>) {
        return U{};
    }
}

template<typename T, template<typename> class TaskImpl, typename PromiseType>
template<typename TaskT, typename ThenCallback, typename ErrorCallback, typename R>
inline auto TaskBase<T, TaskImpl, PromiseType>::thenImpl(TaskT task_, ThenCallback &&thenCallback, ErrorCallback &&errorCallback) -> std::conditional_t<detail::isTask_v<R>, R, TaskImpl<R>> {
    auto thenCb = std::forward<ThenCallback>(thenCallback);
    auto errCb = std::forward<ErrorCallback>(errorCallback);
    const auto &task = static_cast<const TaskImpl<T> &>(task_);

    if constexpr (std::is_void_v<typename TaskImpl<T>::value_type>) {
        try {
            co_await task;
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
            value.emplace(std::move(co_await task));
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

template<typename T, template<typename> class TaskImpl, typename PromiseType>
template<typename TaskT, typename ThenCallback, typename ErrorCallback, typename R>
inline auto TaskBase<T, TaskImpl, PromiseType>::thenImplRef(TaskT &task_, ThenCallback &&thenCallback, ErrorCallback &&errorCallback) -> std::conditional_t<detail::isTask_v<R>, R, TaskImpl<R>> {
    auto thenCb = std::forward<ThenCallback>(thenCallback);
    auto errCb = std::forward<ErrorCallback>(errorCallback);
    const auto &task = static_cast<const TaskImpl<T> &>(task_);

    if constexpr (std::is_void_v<typename TaskImpl<T>::value_type>) {
        try {
            co_await task;
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
            value.emplace(std::move(co_await task));
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



} // namespace QCoro::detail
