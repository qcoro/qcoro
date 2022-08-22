// SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "coroutine.h"
#include "concepts_p.h"
#include "qcorotask.h"

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

template<typename T>
class Task;

/*! \cond internal */

namespace detail {

template<typename T>
struct awaiter_type;

template<typename T>
using awaiter_type_t = typename awaiter_type<T>::type;

class CancellableTaskBase;


class CancellableTaskPromiseBase : public PromiseBase {
public:
    template<typename T, typename Awaiter = QCoro::detail::awaiter_type_t<std::remove_cvref_t<T>>>
    auto await_transform(T &&value) {
        return Awaiter{value};
    }

    template<typename T>
    auto &await_transform(QCoro::CancellableTask<T> &&task) {
        setAwaitedTask(std::move(task));
        return static_cast<QCoro::CancellableTask<T> &>(*awaitedTask());
    }

    //! \copydoc template<typename T> QCoro::TaskPromiseBase::await_transform(QCoro::Task<T> &&)
    template<typename T>
    auto &await_transform(QCoro::CancellableTask<T> &task) {
        setAwaitedTask(task);
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

    void setAwaitedTask(detail::CancellableTaskBase &task) {
        mAwaitedTask = task;
    }

    template<typename T>
    void setAwaitedTask(CancellableTask<T> &&task) {
        mAwaitedTask = std::make_unique<CancellableTask<T>>(std::move(task));
    }

    CancellableTaskBase *awaitedTask() const {
        if (std::holds_alternative<std::monostate>(mAwaitedTask)) {
            return nullptr;
        } else if (std::holds_alternative<std::reference_wrapper<detail::CancellableTaskBase>>(mAwaitedTask)) {
            return std::addressof(std::get<std::reference_wrapper<detail::CancellableTaskBase>>(mAwaitedTask).get());
        } else if (std::holds_alternative<std::unique_ptr<detail::CancellableTaskBase>>(mAwaitedTask)) {
            return std::get<std::unique_ptr<detail::CancellableTaskBase>>(mAwaitedTask).get();
        } else {
            Q_UNREACHABLE();
        }
    }

protected:
    bool mCancelled = false;
    std::variant<std::monostate, std::reference_wrapper<detail::CancellableTaskBase>, std::unique_ptr<detail::CancellableTaskBase>> mAwaitedTask;
};

template<typename T>
struct isResult : public std::false_type {};

template<typename T>
struct isResult<QCoro::Result<T>> : public std::true_type {};

template<typename T>
static constexpr bool isResult_v = isResult<std::remove_cvref_t<T>>::value;

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
        mValue = Result<T>{std::forward<T>(value)};
    }

    void return_value(const T &value) noexcept {
        mValue = Result<T>{value};
    }

    template<typename U>
    requires (std::is_constructible_v<T, U> && !isResult_v<U>)
    void return_value(U &&value) noexcept {
        mValue = Result<T>{std::forward<U>(value)};
    }

    template<typename U>
    requires (std::is_constructible_v<T, U> && !isResult_v<U>)
    void return_value(const U &value) noexcept {
        mValue = Result<T>{value};
    }

    void return_value(Result<T> &&result) noexcept {
        mValue = std::move(result);
    }

    void return_value(const Result<T> &result) noexcept {
        if (result.isCancelled()) {
            mValue = Result<T>(cancellation());
        } else {
            mValue = Result<T>(*result);
        }
    }

    template<typename U>
    requires std::is_constructible_v<T, U>
    void return_value(Result<U> &&result) noexcept {
        mValue = std::move(result);
    }

    template<typename U>
    requires std::is_constructible_v<T, U>
    void return_value(const Result<U> &result) noexcept {
        if (result.isCancelled()) {
            mValue.template emplace<Result<T>>(cancellation());
        } else {
            mValue.template emplace<Result<T>>(*result);
        }
    }

    Result<T> &&result() {
        if (std::holds_alternative<std::exception_ptr>(mValue)) {
            Q_ASSERT(std::get<std::exception_ptr>(mValue) != nullptr);
            std::rethrow_exception(std::get<std::exception_ptr>(mValue));
        }

        if (mCancelled) {
            mValue = Result<T>{detail::cancellation()};
        }
        return std::move(std::get<Result<T>>(mValue));
    }

private:
    std::variant<std::monostate, Result<T>, std::exception_ptr> mValue;
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

template<typename T>
struct isCancellableTask : std::false_type {
    using return_type = Result<T>;
};

template<typename T>
struct isCancellableTask<QCoro::CancellableTask<T>> : std::true_type {
    using return_type = Result<typename QCoro::CancellableTask<T>::value_type>;
};

template<typename T>
constexpr bool isCancellableTask_v = isCancellableTask<T>::value;

class CancellableTaskBase {
    template<typename T> friend class QCoro::CancellableTask;

protected:
    explicit CancellableTaskBase() = default;
    explicit CancellableTaskBase(std::coroutine_handle<> coroutine)
        : mCoroutine(coroutine)
    {}

    CancellableTaskBase(const CancellableTaskBase &) = delete;
    CancellableTaskBase &operator=(const CancellableTaskBase &) = delete;

    CancellableTaskBase(CancellableTaskBase &&other) noexcept
        : mCoroutine(other.mCoroutine)
    {
        other.mCoroutine = nullptr;
    }

public:
    virtual ~CancellableTaskBase() = default;

    bool isReady() const {
        return !mCoroutine || mCoroutine.done();
    }

    virtual void cancel() = 0;

    void destroy() {
        if (!isReady()) {
            mCoroutine.destroy();
        }
    }

protected:
    virtual void cancelInner() = 0;

    //! The coroutine represented by this task
    /*!
     * In other words, this is a handle to the coroutine that has constructed and
     * returned this Task<T>.
     * */
    std::coroutine_handle<> mCoroutine = {};
};

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
class CancellableTask final : public detail::CancellableTaskBase {
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
    explicit CancellableTask(std::coroutine_handle<promise_type> coroutine)
        : detail::CancellableTaskBase(coroutine) {}

    //! Task cannot be copy-constructed.
    CancellableTask(const CancellableTask &) = delete;
    //! Task cannot be copy-assigned.
    CancellableTask &operator=(const CancellableTask &) = delete;

    //! The task can be move-constructed.
    CancellableTask(CancellableTask &&other) noexcept = default;

    //! The task can be move-assigned.
    CancellableTask &operator=(CancellableTask &&other) noexcept {
        if (std::addressof(other) != this) {
            if (mCoroutine) {
                // The coroutine handle will be destroyed only after TaskFinalSuspend
                if (promise().setDestroyHandle()) {
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
        if (promise().setDestroyHandle()) {
            mCoroutine.destroy();
        }
    };

    void cancelInner() override {
        if (isReady()) {
            return;
        }

        if (promise().awaitedTask()) {
            promise().awaitedTask()->cancelInner();
        }

        // TaskFinalSuspend will not be called, so it's up to the CancellableTask dtor to
        // destroy the coroutine state.
        promise().setDestroyHandle();
        promise().setCancelled(); // Mark the promise is cancelled
    }

    void cancel() override {
        if (isReady()) {
            return;
        }

        cancelInner();

        // Resume awaiter of the top-level cancelled coroutine
        if (promise().mAwaitingCoroutine) {
            promise().mAwaitingCoroutine.resume();  // Resume an awaiter, if there's any
        }
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
                : detail::TaskAwaiterBase<promise_type>{awaitedCoroutine} {}

            //! Called when the co_awaited coroutine is resumed.
            /*!
             * \return the result from the coroutine's promise, factically the
             * value co_returned by the coroutine. */
            auto await_resume() {
                Q_ASSERT(this->mAwaitedCoroutine != nullptr);
                return std::move(this->mAwaitedCoroutine.promise().result());
            }
        };

        return CancellableTaskAwaiter{
            std::coroutine_handle<promise_type>::from_address(mCoroutine.address())
        };
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
        // Provide a custom error handler that simply re-throws the current exception
        return thenImpl(std::forward<ThenCallback>(callback), [](const auto &) { throw; });
    }

    template<typename ThenCallback, typename ErrorCallback>
    requires ((std::is_invocable_v<ThenCallback> || (!std::is_void_v<T> && std::is_invocable_v<ThenCallback, T>)) &&
               std::is_invocable_v<ErrorCallback, const std::exception &>)
    auto then(ThenCallback &&callback, ErrorCallback &&errorCallback) {
        return thenImpl(std::forward<ThenCallback>(callback), std::forward<ErrorCallback>(errorCallback));
    }

private:
    inline promise_type &promise() {
        return std::coroutine_handle<promise_type>::from_address(mCoroutine.address()).promise();
    }

    template<typename ThenCallback, typename ... Args>
    auto invokeCb(ThenCallback &&callback, Args && ... args) {
        if constexpr (std::is_invocable_v<ThenCallback, Args ...>) {
            return callback(std::forward<Args>(args) ...);
        } else {
            return callback();
        }
    }

    template<typename ThenCallback, typename Arg>
    struct cb_invoke_result: std::conditional_t<
        std::is_invocable_v<ThenCallback>,
            std::invoke_result<ThenCallback>,
            std::invoke_result<ThenCallback, Arg>
        > {};

    template<typename ThenCallback>
    struct cb_invoke_result<ThenCallback, void>: std::invoke_result<ThenCallback> {};

    template<typename ThenCallback, typename Arg>
    using cb_invoke_result_t = typename cb_invoke_result<ThenCallback, Arg>::type;

    template<typename R, typename ErrorCallback,
             typename U = typename detail::isCancellableTask<R>::return_type>
    auto handleException(ErrorCallback &errCb, const std::exception &exception) -> U {
        errCb(exception);
        if constexpr (!std::is_void_v<U>) {
            return U{};
        }
    }

    template<typename ThenCallback, typename ErrorCallback, typename R = cb_invoke_result_t<ThenCallback, T>>
    auto thenImpl(ThenCallback &&thenCallback, ErrorCallback &&errorCallback) -> std::conditional_t<detail::isCancellableTask_v<R>, R, CancellableTask<R>> {
        const auto thenCb = std::forward<ThenCallback>(thenCallback);
        const auto errCb = std::forward<ErrorCallback>(errorCallback);
        if constexpr (std::is_void_v<value_type>) {
            try {
                auto result = co_await *this;
                if (result.isCancelled()) {
                    co_return result;
                }
            } catch (const std::exception &e) {
                co_return handleException<R>(errCb, e);
            }
            if constexpr (detail::isCancellableTask_v<R>) {
                co_return co_await invokeCb(thenCb);
            } else {
                co_return invokeCb(thenCb);
            }
        } else {
            std::optional<T> value;
            try {
                auto result = co_await *this;
                if (result.isCancelled()) {
                    co_return std::move(result);
                }
                value = std::move(*result);
            } catch (const std::exception &e) {
                co_return handleException<R>(errCb, e);
            }
            if constexpr (detail::isCancellableTask_v<R>) {
                co_return co_await invokeCb(thenCb, std::move(*value));
            } else {
                co_return invokeCb(thenCb, std::move(*value));
            }
        }
    }
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
inline Result<T> waitFor(QCoro::CancellableTask<T> &task) {
    return detail::waitFor<T>(task);
}

// \overload
template<typename T>
inline Result<T> waitFor(QCoro::CancellableTask<T> &&task) {
    return detail::waitFor<T>(task);
}

} // namespace QCoro
