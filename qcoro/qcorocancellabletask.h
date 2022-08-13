// SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "coroutine.h"
#include "concepts_p.h"

#include "bits/cancellation.h"
#include "bits/result.h"
#include "bits/taskfinalsuspend.h"
#include "bits/promisebase.h"
#include "bits/taskawaiterbase.h"

#include <atomic>
#include <variant>
#include <memory>
#include <type_traits>
#include <optional>

#include <QDebug>
#include <QEventLoop>
#include <QObject>
#include <QCoreApplication>

namespace QCoro {

template<typename T = void>
class CancellableTask;

template<typename T = void>
class DetachedTask;

template<typename T = void>
class Task;

/*! \cond internal */

namespace detail {

template<typename T>
struct awaiter_type;

template<typename T>
using awaiter_type_t = typename awaiter_type<T>::type;

class CancellableTaskPromiseBase : public PromiseBase {
public:
    template<typename T, typename Awaiter = QCoro::detail::awaiter_type_t<std::remove_cvref_t<T>>>
    auto await_transform(T &&value) {
        return Awaiter{value};
    }

    template<typename T>
    auto await_transform(QCoro::CancellableTask<T> &&task) {
        return std::forward<QCoro::CancellableTask<T>>(task);
    }

    //! \copydoc template<typename T> QCoro::TaskPromiseBase::await_transform(QCoro::Task<T> &&)
    template<typename T>
    auto &await_transform(QCoro::CancellableTask<T> &task) {
        return task;
    }

    //! Allow detached non-cancellable task to be co_awaited inside a cancellable task.
    template<typename T>
    auto await_transform(QCoro::DetachedTask<T> &&task) {
        return std::forward<QCoro::DetachedTask<T>>(task);
    }

    //! Allow detached non-cancellable task to be co_awaited inside a cancellable task.
    template<typename T>
    auto &await_transform(QCoro::DetachedTask<T> &task) {
        return task;
    }

    //! It's not possible to co_await a Task inside a CancellableTask.
    /*!
     * If you really need to co_await a non-cancellable Task inside a CancellableTask,
     * use QCoro::detachTask() to explicitly detach the non-cancellable Task from the
     * current cancellable task. Remember that if the current task is cancelled,
     * the detached task will not be cancelled which may lead to memory leaks.
     */
    template<typename T>
    void await_transform(QCoro::Task<T> &) = delete;

    template<Awaitable T>
    auto && await_transform(T &&awaitable) {
        return std::forward<T>(awaitable);
    }

    template<Awaitable T>
    auto &await_transform(T &awaitable) {
        return awaitable;
    }

    void setCancelled() {
        mCancelled = true;
    }

protected:
    bool mCancelled = false;
};

template<typename T>
class CancellableTaskPromise final : public CancellableTaskPromiseBase {
public:
    explicit CancellableTaskPromise() = default;
    Q_DISABLE_COPY_MOVE(CancellableTaskPromise)
    ~CancellableTaskPromise() = default;

    CancellableTask<T> get_return_object() noexcept;

    void unhandled_exception() {
        mValue = std::current_exception();
    }

    void return_value(T &&value) noexcept {
        mValue.template emplace<T>(std::forward<T>(value));
    }

    void return_value(const T &value) noexcept {
        mValue = value;
    }

    template<typename U> requires QCoro::concepts::constructible_from<T, U>
    void return_value(U &&value) noexcept {
        mValue = std::forward<U>(value);
    }

    template<typename U> requires QCoro::concepts::constructible_from<T, U>
    void return_value(const U &value) noexcept {
        mValue = value;
    }

    Result<T> result() {
        if (std::holds_alternative<std::exception_ptr>(mValue)) {
            Q_ASSERT(std::get<std::exception_ptr>(mValue) != nullptr);
            std::rethrow_exception(std::get<std::exception_ptr>(mValue));
        }

        if (mCancelled) {
            return Result<T>{detail::cancellation()};
        }
        return Result<T>{std::move(std::get<T>(mValue))};
    }

private:
    std::variant<std::monostate, T, std::exception_ptr> mValue;
};

template<>
class CancellableTaskPromise<void> final : public CancellableTaskPromiseBase {
public:
    explicit CancellableTaskPromise() = default;
    Q_DISABLE_COPY_MOVE(CancellableTaskPromise)
    ~CancellableTaskPromise() = default;

    CancellableTask<void> get_return_object() noexcept;

    void unhandled_exception() {
        mException = std::current_exception();
    }

    void return_void() noexcept {};

    Result<void> result() {
        if (mException) {
            std::rethrow_exception(mException);
        }

        if (mCancelled) {
            return Result<void>{detail::cancellation()};
        }
        return Result<void>{};
    }

private:
    std::exception_ptr mException;
};

template<typename>
struct isCancellableTask : public std::false_type {};

template<typename T>
struct isCancellableTask<QCoro::CancellableTask<T>> : public std::true_type {};

} // namespace detail

/*! \endcond */

//! An asynchronously executed task.
/*!
 * When a coroutine is called which has  return type Task<T>, the coroutine will
 * construct a new instance of Task<T> which will be returned to the caller when
 * the coroutine is suspended - that is either when it co_awaits another coroutine
 * of finishes executing user code.
 *
 * In the sense of the interface that the task implements, it is an Awaitable.
 *
 * Task<T> is constructed by a code generated at the beginning of the coroutine by
 * the compiler (i.e. before the user code). The code first creates a new frame
 * pointer, which internally holds the promise. The promise is of type \c R::promise_type,
 * where \c R is the return type of the function (so \c Task<T>, in our case.
 *
 * One can think about it as Task being the caller-facing interface and promise being
 * the callee-facing interface.
 */
template<typename T>
class CancellableTask {
public:
    //! Promise type of the coroutine. This is required by the C++ standard.
    using promise_type = detail::CancellableTaskPromise<T>;
    //! The type of the coroutine return value.
    using value_type = T;

    //! Constructs a new empty task.
    explicit CancellableTask() noexcept = default;

    //! Constructs a task bound to a coroutine.
    /*!
     * \param[in] coroutine handle of the coroutine that has constructed the task.
     */
    explicit CancellableTask(std::coroutine_handle<promise_type> coroutine) : mCoroutine(coroutine) {}

    //! Task cannot be copy-constructed.
    CancellableTask(const CancellableTask &) = delete;
    //! Task cannot be copy-assigned.
    CancellableTask &operator=(const CancellableTask &) = delete;

    //! The task can be move-constructed.
    CancellableTask(CancellableTask &&other) noexcept : mCoroutine(other.mCoroutine) {
        other.mCoroutine = nullptr;
    }

    //! The task can be move-assigned.
    CancellableTask &operator=(CancellableTask &&other) noexcept {
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

    //! Destructor.
    ~CancellableTask() {
        if (!mCoroutine) return;

        // The coroutine handle will be destroyed only after TaskFinalSuspend
        if (mCoroutine.promise().setDestroyHandle()) {
            mCoroutine.destroy();
        }
    };

    //! Returns whether the task has finished.
    /*!
     * A task that is ready (represents a finished coroutine) must not attempt
     * to suspend the coroutine again.
     */
    bool isReady() const {
        return !mCoroutine || mCoroutine.done();
    }

    void cancel() {
        if (isReady()) {
            return;
        }

        /* TODO: Handle inner coroutines here
        if (mCoroutine.promise().awaitedCoroutine) {
            mCoroutine.promise().awaitedCoroutine.destroy();
        }
        */

        mCoroutine.promise().setCancelled();
        mCoroutine.destroy();
    }

    //! Provides an Awaiter for the coroutine machinery.
    /*!
     * The coroutine machinery looks for a suitable operator co_await overload
     * for the current Awaitable (this Task). It calls it to obtain an Awaiter
     * object, that is an object that the co_await keyword uses to suspend and
     * resume the coroutine.
     */
    auto operator co_await() const noexcept {
        //! Specialization of the TaskAwaiterBase that returns the promise result by value
        class CancellableTaskAwaiter : public detail::TaskAwaiterBase<promise_type> {
        public:
            explicit CancellableTaskAwaiter(std::coroutine_handle<promise_type> awaitedCoroutine)
                : detail::CancellableTaskAwaiterBase<promise_type>{awaitedCoroutine} {}

            //! Called when the co_awaited coroutine is resumed.
            /*!
             * \return the result from the coroutine's promise, factically the
             * value co_returned by the coroutine. */
            auto await_resume() {
                Q_ASSERT(this->mAwaitedCoroutine != nullptr);
                return std::move(this->mAwaitedCoroutine.promise().result());
            }
        };

        return CancellableTaskAwaiter{mCoroutine};
    }

    //! A callback to be invoked when the asynchronous task finishes.
    /*!
     * In some scenarios it is not possible to co_await a coroutine (for example from
     * a third-party code that cannot be changed to be a coroutine). In that case,
     * chaining a then() callback is a possible solution how to handle a result
     * of a coroutine without co_awaiting it.
     *
     * @param callback A function or a function object that can be invoked without arguments
     * (discarding the result of the coroutine) or with a single argument of type T that is
     * type matching the return type of the coroutine or is implicitly constructible from
     * the return type returned by the coroutine.
     *
     * @return Returns Task<R> where R is the return type of the callback, so that the
     * result of the then() action can be co_awaited, if desired. If the callback
     * returns an awaitable (Task<R>) then the result of then is the awaitable.
     */
    template<typename ThenCallback>
    requires (std::is_invocable_v<ThenCallback> || (!std::is_void_v<T> && std::is_invocable_v<ThenCallback, T>))
    auto then(ThenCallback &&callback) {
        return thenImpl(std::forward<ThenCallback>(callback));
    }

    template<typename ThenCallback, typename ErrorCallback>
    requires ((std::is_invocable_v<ThenCallback> || (!std::is_void_v<T> && std::is_invocable_v<ThenCallback, T>)) &&
               std::is_invocable_v<ErrorCallback, const std::exception &>)
    auto then(ThenCallback &&callback, ErrorCallback &&errorCallback) {
        return thenImpl(std::forward<ThenCallback>(callback), std::forward<ErrorCallback>(errorCallback));
    }

private:
    template<typename ThenCallback, typename ... Args>
    requires (std::is_invocable_v<ThenCallback>)
    auto invoke(ThenCallback &&callback, Args && ...) {
        return callback();
    }

    template<typename ThenCallback, typename Arg>
    requires (std::is_invocable_v<ThenCallback, Arg>)
    auto invoke(ThenCallback &&callback, Arg && arg) {
        return callback(std::forward<Arg>(arg));
    }

    template<typename ThenCallback, typename Arg>
    struct invoke_result: std::conditional_t<
        std::is_invocable_v<ThenCallback>,
            std::invoke_result<ThenCallback>,
            std::invoke_result<ThenCallback, Arg>
        > {};

    template<typename ThenCallback>
    struct invoke_result<ThenCallback, void>: std::invoke_result<ThenCallback> {};

    template<typename ThenCallback, typename Arg>

    using invoke_result_t = typename invoke_result<ThenCallback, Arg>::type;

    // Implementation of then() for callbacks that return Task<R>
    template<typename ThenCallback, typename R = invoke_result_t<ThenCallback, T>>
    requires detail::isCancellableTask<R>::value
    auto thenImpl(ThenCallback &&callback) -> R {
        const auto cb = std::move(callback);
        auto result = co_await *this;
        if (result.isCancelled()) {
            co_return detail::cancellation();
        }
        if constexpr (std::is_void_v<value_type>) {
            co_return co_await invoke(cb);
        } else {
            co_return co_await invoke(cb, std::move(*result));
        }
    }

    template<typename ThenCallback, typename ErrorCallback, typename R = invoke_result_t<ThenCallback, T>>
    requires detail::isCancellableTask<R>::value
    auto thenImpl(ThenCallback &&thenCallback, ErrorCallback &&errorCallback) -> R {
        const auto thenCb = std::move(thenCallback);
        const auto errCb = std::move(errorCallback);
        if constexpr (std::is_void_v<value_type>) {
            try {
                auto result = co_await *this;
                if (result.isCancelled()) {
                    co_return detail::cancellation();
                }
            } catch (const std::exception &e) {
                errCb(e);
                if constexpr (!std::is_void_v<typename R::value_type>) {
                    co_return {};
                } else {
                    co_return;
                }
            }
            co_return co_await invoke(thenCb);
        } else {
            std::optional<T> v;
            try {
                auto result = co_await *this;
                if (result.isCancelled()) {
                    co_return detail::cancellation();
                }
                v = std::move(*result);
            } catch (const std::exception &e) {
                errCb(e);
                if constexpr (!std::is_void_v<typename R::value_type>) {
                    co_return {};
                } else {
                    co_return;
                }
            }
            co_return co_await invoke(thenCb, std::move(*v));
        }
    }


    // Implementation of then() for callbacks that return R, which is not Task<S>
    template<typename ThenCallback, typename R = invoke_result_t<ThenCallback, T>>
    requires (!detail::isCancellableTask<R>::value)
    auto thenImpl(ThenCallback &&callback) -> Task<R> {
        const auto cb = std::move(callback);
        auto result = co_await *this;
        if (result.isCancelled()) {
            co_return detail::cancellation();
        }
        if constexpr (std::is_void_v<value_type>) {
            co_return invoke(cb);
        } else {
            co_return invoke(cb, std::move(*result));
        }
    }

    template<typename ThenCallback, typename ErrorCallback, typename R = invoke_result_t<ThenCallback, T>>
    requires (!detail::isCancellableTask<R>::value)
    auto thenImpl(ThenCallback &&thenCallback, ErrorCallback &&errorCallback) -> Task<R> {
        const auto thenCb = std::move(thenCallback);
        const auto errCb = std::move(errorCallback);
        if constexpr (std::is_void_v<value_type>) {
            try {
                auto result = co_await *this;
                if (result.isCancelled()) {
                    co_return detail::cancellation();
                }
            } catch (const std::exception &e) {
                errCb(e);
                if constexpr (!std::is_void_v<R>) {
                    co_return {};
                } else {
                    co_return;
                }
            }
            co_return invoke(thenCb);
        } else {
            std::optional<T> v;
            try {
                auto result = co_await *this;
                if (result.isCancelled()) {
                    co_return detail::cancellation();
                }
                v = std::move(*result);
            } catch (const std::exception &e) {
                errCb(e);
                if constexpr (!std::is_void_v<R>) {
                    co_return {};
                } else {
                    co_return;
                }
            }
            co_return invoke(thenCb, std::move(*v));
        }
    }

private:
    //! The coroutine represented by this task
    /*!
     * In other words, this is a handle to the coroutine that has constructed and
     * returned this Task<T>.
     * */
    std::coroutine_handle<promise_type> mCoroutine = {};
};

namespace detail {

template<typename T>
inline CancellableTask<T> CancellableTaskPromise<T>::get_return_object() noexcept {
    return CancellableTask<T>{std::coroutine_handle<CancellableTaskPromise>::from_promise(*this)};
}

inline CancellableTask<void> CancellableTaskPromise<void>::get_return_object() noexcept {
    return CancellableTask<void>{std::coroutine_handle<CancellableTaskPromise>::from_promise(*this)};
}

} // namespace detail


namespace detail {

//! Helper class to run a coroutine in a nested event loop.
/*!
 * We cannot just use QTimer or QMetaObject::invokeMethod() to schedule the func lambda to be
 * invoked from an event loop, because internally, Qt deallocates some structures when the
 * lambda returns, which causes invalid memory access and potentially double-free corruption
 * because the coroutine returns twice - once on suspend and once when it really finishes.
 * So instead we do basically what Qt does internally, but we make sure to not delete the
 * QFunctorSlotObjectWithNoArgs until after the event loop quits.
 */
template<typename Coroutine>
CancellableTask<> runCancellableCoroutine(QEventLoop &loop, bool &startLoop, Coroutine &&coroutine) {
    co_await coroutine;
    startLoop = false;
    loop.quit();
}

template<typename T, typename Coroutine>
CancellableTask<> runCancellableCoroutine(QEventLoop &loop, bool &startLoop, T &result, Coroutine &&coroutine) {
    result = co_await coroutine;
    startLoop = false;
    loop.quit();
}

template<typename T, typename Coroutine>
Result<T> waitFor(Coroutine &&coro) {
    QEventLoop loop;
    bool startLoop = true; // for early returns: calling quit() before exec() still starts the loop
    if constexpr (std::is_void_v<T>) {
        runCancellableCoroutine(loop, startLoop, std::forward<Coroutine>(coro));
        if (startLoop) {
            loop.exec();
        }
    } else {
        Result<T> result;
        runCancellableCoroutine(loop, startLoop, result, std::forward<Coroutine>(coro));
        if (startLoop) {
            loop.exec();
        }
        return result;
    }
}

} // namespace detail

//! Waits for a coroutine to complete in a blocking manner.
/*!
 * Sometimes you may need to wait for a coroutine to finish  without co_awaiting it - that is,
 * you want to wait for the coroutine in a blocking mode. This function does exactly that.
 * The function creates a nested QEventLoop and executes it until the coroutine has finished.
 *
 * \param task Coroutine to blockingly wait for.
 * \returns Result of the coroutine.
 */
template<typename T>
inline T waitFor(QCoro::CancellableTask<T> &task) {
    return detail::waitFor<T>(task);
}

// \overload
template<typename T>
inline T waitFor(QCoro::CancellableTask<T> &&task) {
    return detail::waitFor<T>(task);
}

} // namespace QCoro
