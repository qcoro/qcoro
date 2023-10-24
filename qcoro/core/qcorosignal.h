// SPDX-FileCopyrightText: 2021-2023 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "coroutine.h"
#include "macros_p.h"
#include "concepts_p.h"
#include "qcorotask.h"
#include "qcoroasyncgenerator.h"

#include <QObject>
#include <QPointer>
#include <QTimer>

#include <cassert>
#include <optional>
#include <deque>
#include <tuple>
#include <type_traits>

#include "impl/isqprivatesignal.h"

namespace QCoro::detail {

namespace concepts {

//! Simplistic QObject concept.
template<typename T>
concept QObject = requires(T *obj) {
    requires std::is_base_of_v<::QObject, T>;
    requires std::is_same_v<decltype(T::staticMetaObject), const QMetaObject>;
};

} // namespace concepts

template<concepts::QObject T, typename FuncPtr>
class QCoroSignalBase {
private:
    template<typename T1, typename T2>
    struct concat_tuple;

    template<typename Arg, typename... TupleTypes>
    struct concat_tuple<Arg, std::tuple<TupleTypes...>> {
        using type = std::tuple<TupleTypes..., Arg>;
    };

    template<typename Tuple, typename... Args>
    struct filtered_tuple;

    template<typename Tuple, typename Arg, typename... Args>
    struct filtered_tuple<Tuple, Arg, Args...> {
        using type = std::conditional_t<
            is_qprivatesignal_v<Arg>, typename filtered_tuple<Tuple, Args...>::type,
            typename filtered_tuple<typename concat_tuple<Arg, Tuple>::type, Args...>::type>;
    };

    template<typename Tuple>
    struct filtered_tuple<Tuple> {
        using type = Tuple;
    };

    template<class>
    struct args_tuple;

    template<class R, class... Args>
    struct args_tuple<R(Args...)> {
        using type = std::tuple<std::remove_cvref_t<Args>...>;
    };

    template<class R, class Obj, class... Args>
    struct args_tuple<R (Obj::*)(Args...)> {
        using type = typename filtered_tuple<std::tuple<>, std::remove_cvref_t<Args>...>::type;
    };

    template<typename Arg>
    struct result_type_from_tuple;

    template<typename Arg>
    struct result_type_from_tuple<std::tuple<Arg>> {
        using type = Arg;
    };

    template<typename... Args>
    struct result_type_from_tuple<std::tuple<Args...>> {
        using type = std::tuple<Args...>;
    };

    using result_tuple = typename args_tuple<std::remove_cvref_t<FuncPtr>>::type;

public:
    /**!
     * The result_type is std::optional of
     *  * T if result_tuple is std::tuple<T>
     *  * result_tuple otherwise
     **/
    using result_type = std::optional<typename result_type_from_tuple<result_tuple>::type>;

    QCoroSignalBase(const QCoroSignalBase &) = delete;
    QCoroSignalBase &operator=(const QCoroSignalBase &) = delete;
    QCoroSignalBase(QCoroSignalBase &&) noexcept = default;
    QCoroSignalBase &operator=(QCoroSignalBase &&) noexcept = default;

    ~QCoroSignalBase() {
        if (static_cast<bool>(mConn)) {
            QObject::disconnect(mConn);
        }
    }

    void handleTimeout(std::coroutine_handle<> awaitingCoroutine) {
        if (mTimeoutTimer) {
            QObject::connect(mTimeoutTimer.get(), &QTimer::timeout, mObj,
                [this, awaitingCoroutine]() {
                    QObject::disconnect(mConn);
                    awaitingCoroutine.resume();
                });
            mTimeoutTimer->start();
        }
    }

protected:
    QCoroSignalBase(T *obj, FuncPtr &&funcPtr, std::chrono::milliseconds timeout)
        : mObj(obj), mFuncPtr(std::forward<FuncPtr>(funcPtr)) {
        if (timeout.count() > -1) {
            mTimeoutTimer = std::make_unique<QTimer>();
            mTimeoutTimer->setInterval(timeout);
            mTimeoutTimer->setSingleShot(true);
        }
    }

    template<typename... Args>
    struct select_last {
        using type = typename decltype((std::type_identity<Args>{}, ...))::type;
    };

    template<typename StoreResultCb, typename... Args>
    constexpr void storeResult(StoreResultCb &&storeResult, Args &&...args) {
        using LastArg = typename select_last<Args...>::type;
        if constexpr (is_qprivatesignal_v<LastArg>) {
            // Based on https://stackoverflow.com/a/77026174/4601437
            // Remove the last element (which is a QPrivateSignal) from the tuple
            auto all = std::forward_as_tuple(std::forward<Args>(args)...);
            auto reduced = [&]<std::size_t... I>(std::index_sequence<I...>) constexpr {
                return std::make_tuple(std::get<I>(all)...);
            }(std::make_index_sequence<sizeof...(Args) - 1>{});
            // Use the shortened tuple as arguments to mResult.emplace()
            std::apply(std::forward<StoreResultCb>(storeResult), std::move(reduced));
        } else {
            std::invoke(std::forward<StoreResultCb>(storeResult), std::forward<Args>(args)...);
        }
    }

    template<typename StoreResultCb>
    constexpr void storeResult(StoreResultCb &&storeResult) {
        std::invoke(std::forward<StoreResultCb>(storeResult));
    }

protected:
    QPointer<T> mObj;
    FuncPtr mFuncPtr;
    QMetaObject::Connection mConn;
    std::unique_ptr<QTimer> mTimeoutTimer;
};

template<concepts::QObject T, typename FuncPtr>
class QCoroSignal : public QCoroSignalBase<T, FuncPtr> {
public:
    using typename QCoroSignalBase<T, FuncPtr>::result_type;

    QCoroSignal(T *obj, FuncPtr &&ptr, std::chrono::milliseconds timeout)
        : QCoroSignalBase<T, FuncPtr>(obj, std::forward<FuncPtr>(ptr), timeout)
        , mDummyReceiver(std::make_unique<QObject>()) {}
    QCoroSignal(const QCoroSignal &) = delete;
    QCoroSignal(QCoroSignal &&other) noexcept
        : QCoroSignalBase<T, FuncPtr>(std::move(other))
        , mResult(std::move(other.mResult))
        , mDummyReceiver(std::move(other.mDummyReceiver)) {
        if (this->mConn) {
            QObject::disconnect(this->mConn);
            setupConnection();
        }
    }

    QCoroSignal &operator=(QCoroSignal &&other) noexcept {
        QCoroSignalBase<T, FuncPtr>::operator=(std::move(other));
        std::swap(mResult, other.mResult);
        std::swap(mDummyReceiver, other.mDummyReceiver);
        if (this->mConn) {
            QObject::disconnect(this->mConn);
            setupConnection();
        }
        return *this;
    }

    QCoroSignal &operator=(const QCoroSignal &) = delete;
    ~QCoroSignal() = default;


    bool await_ready() const noexcept {
        return this->mObj.isNull();
    }

    void await_suspend(std::coroutine_handle<> awaitingCoroutine) noexcept {
        this->handleTimeout(awaitingCoroutine);
        mAwaitingCoroutine = awaitingCoroutine;
        setupConnection();
    }

    result_type await_resume() {
        return std::move(mResult);
    }

private:
    void setupConnection() {
        Q_ASSERT(!this->mConn);
        this->mConn = QObject::connect(
            this->mObj, this->mFuncPtr, this->mDummyReceiver.get(),
            [this](auto &&...args) mutable {
                if (this->mTimeoutTimer) {
                    this->mTimeoutTimer->stop();
                }
                QObject::disconnect(this->mConn);

                this->storeResult([this](auto && ...args) {
                    mResult.emplace(std::forward<decltype(args)>(args)...);
                }, std::forward<decltype(args)>(args)...);

                if (mAwaitingCoroutine) {
                    mAwaitingCoroutine.resume();
                }
            },
            Qt::QueuedConnection);
    }

    result_type mResult;
    std::coroutine_handle<> mAwaitingCoroutine;
    std::unique_ptr<QObject> mDummyReceiver;
};

template<concepts::QObject T, typename FuncPtr>
QCoroSignal(T *, FuncPtr &&, std::chrono::milliseconds) -> QCoroSignal<T, FuncPtr>;

template<concepts::QObject T, typename FuncPtr>
class QCoroSignalQueue : public QCoroSignalBase<T, FuncPtr> {
public:
    using typename QCoroSignalBase<T, FuncPtr>::result_type;

    QCoroSignalQueue(T *obj, FuncPtr &&ptr, std::chrono::milliseconds timeout)
        : QCoroSignalBase<T, FuncPtr>(obj, std::forward<FuncPtr>(ptr), timeout) {
        setupConnection();
    }

    QCoroSignalQueue(QCoroSignalQueue &&) = delete;
    QCoroSignalQueue(const QCoroSignalQueue &) = delete;
    QCoroSignalQueue &operator=(QCoroSignalQueue &&) = delete;
    QCoroSignalQueue &operator=(const QCoroSignalQueue &) = delete;
    ~QCoroSignalQueue() = default;

    auto operator co_await() noexcept {
        struct Awaiter {
            explicit Awaiter(QCoroSignalQueue &queue)
                : mQueue(queue)
            {}

            bool await_ready() const noexcept {
                return !mQueue.isValid() || !mQueue.empty();
            }
            void await_suspend(std::coroutine_handle<> awaitingCoroutine) noexcept {
                mQueue.handleTimeout(awaitingCoroutine);
                mQueue.setAwaiter(awaitingCoroutine);
            }
            result_type await_resume() {
                return mQueue.dequeue();
            }

        private:
            QCoro::detail::QCoroSignalQueue<T, FuncPtr> &mQueue;
        };
        return Awaiter{*this};
    }

    bool isValid() const {
        return !this->mObj.isNull();
    }

    bool empty() const {
        return mQueue.empty();
    }

    result_type dequeue() {
        if (mQueue.empty()) {
            return std::nullopt;
        }

        auto result = std::move(mQueue.front());
        mQueue.pop_front();
        return result;
    }

    void setAwaiter(std::coroutine_handle<> awaiter) {
        mAwaitingCoroutine = awaiter;
    }

private:
    void setupConnection() {
        if (this->mConn) {
            return;
        }
        this->mConn = QObject::connect(
            this->mObj, this->mFuncPtr, &this->mDummyReceiver,
            [this](auto && ...args) mutable {
                if (this->mTimeoutTimer) {
                    this->mTimeoutTimer->stop();
                }

                this->storeResult([this](auto && ...args) {
                    mQueue.emplace_back(std::forward<decltype(args)>(args)...);
                }, std::forward<decltype(args)>(args) ...);

                if (mAwaitingCoroutine) {
                    mAwaitingCoroutine.resume();
                }
            }, Qt::QueuedConnection);

    }

    std::coroutine_handle<> mAwaitingCoroutine;
    std::deque<typename result_type::value_type> mQueue;
    QObject mDummyReceiver;
};

template<concepts::QObject T, typename FuncPtr>
QCoroSignalQueue(T *, FuncPtr &&, std::chrono::milliseconds) -> QCoroSignalQueue<T, FuncPtr>;


} // namespace QCoro::detail

//! Allows co_awaiting on signal emission with a timeout.
/*!
 * Returns an Awaitable object that allows co_awaiting for a singal
 * to be emitted within the specified timeout. If the signal has exactly
 * one argument, then the value of the argument is returned as a result
 * of awaiting the coroutine. If the signal has two or more arguments,
 * then the arguments are returned as a tuple. If the signal has no
 * arguments, then the result of the coroutine is an empty tuple.
 *
 * If the timeout occurs before the signal is emitted, the result of the
 * coroutine is an empty optional. If the \c timeout is -1 the operation
 * will never time out.
 */
template<QCoro::detail::concepts::QObject T, typename FuncPtr>
inline auto qCoro(T *obj, FuncPtr &&ptr, std::chrono::milliseconds timeout)
    -> QCoro::Task<typename QCoro::detail::QCoroSignal<T, FuncPtr>::result_type> {
    auto result = co_await QCoro::detail::QCoroSignal(obj, std::forward<FuncPtr>(ptr), timeout);
    co_return std::move(result);
}

//! Allows co_awaiting on signal emission.
/*!
 * Returns an Awaitable object that allows co_awaiting for a signal to
 * be emitted. If the signal has exactly one argument, then the value
 * of the argument is returned as a result of awaiting the coroutine.
 * If the signal has two or more arguments, then the arguments are
 * returned as a tuple. If the signal has no arguments, then the result
 * of the coroutine is an empty tuple.
 *
 * @see docs/reference/coro.md
 */
template<QCoro::detail::concepts::QObject T, typename FuncPtr>
inline auto qCoro(T *obj, FuncPtr &&ptr)
    -> QCoro::Task<typename QCoro::detail::QCoroSignal<T, FuncPtr>::result_type::value_type> {
    auto result = co_await qCoro<T, FuncPtr>(obj, std::forward<FuncPtr>(ptr), std::chrono::milliseconds{-1});
    co_return std::move(*result);
}

template<QCoro::detail::concepts::QObject T, typename FuncPtr>
inline auto qCoroSignalListener(T *obj, FuncPtr &&ptr,
                                 std::chrono::milliseconds timeout = std::chrono::milliseconds{-1})
    -> QCoro::AsyncGenerator<typename QCoro::detail::QCoroSignalQueue<T, FuncPtr>::result_type::value_type> {

    using SignalQueue = QCoro::detail::QCoroSignalQueue<T, FuncPtr>;

    // The actual generator is in a wrapper function, so that we can perform
    // some initialization (constructing signalQueue) in the qCoroSignalListener()
    // function before the generator gets initially suspended.
    constexpr auto innerGenerator = [](std::unique_ptr<SignalQueue> signalQueue) ->
        QCoro::AsyncGenerator<typename SignalQueue::result_type::value_type> {
        Q_FOREVER {
            auto result = co_await *signalQueue;
            if (!result.has_value()) { // timeout
                break;
            }

            co_yield std::move(*result);
        }
    };

    return innerGenerator(std::make_unique<SignalQueue>(obj, std::forward<FuncPtr>(ptr), timeout));
}
