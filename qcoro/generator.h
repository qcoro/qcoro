// SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <variant>
#include <exception>

#include "coroutine.h"

namespace QCoro {

template<typename T>
class Generator;

namespace detail {

template<typename T>
class GeneratorPromise {
public:
    Generator<T> get_return_object();

    std::suspend_always initial_suspend() { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }

    void unhandled_exception() {
        mValue = std::current_exception();
    }

    template<typename From>
    requires std::is_convertible_v<From, T>
    std::suspend_always yield_value(From &&from) {
        mValue.template emplace<T>(std::forward<From>(from));
        return {};
    }

    void return_void() {}

    std::exception_ptr exception() const {
        return std::holds_alternative<std::exception_ptr>(mValue)
            ? std::get<std::exception_ptr>(mValue)
            : std::exception_ptr{};
    }

    T && value() {
        T &&v = std::move(std::get<T>(mValue));
        mValue = std::monostate{};
        return std::move(v);
    }

    bool hasValue() const {
        return std::holds_alternative<T>(mValue);
    }

private:
    std::variant<std::monostate, T, std::exception_ptr> mValue;
};

} // namespace detail

template<typename T>
class Generator {
public:
    using promise_type = detail::GeneratorPromise<T>;

    explicit Generator(std::coroutine_handle<promise_type> coroutine)
        : mCoroutine(coroutine)
    {}

    ~Generator() {
        mCoroutine.destroy();
    }

    /**
     * Whether the generator is still running.
     *
     * \returns `true` when the generator has more values to provide or expects
     * a new value to become available. Returns `false` when the generator function
     * has ended.
     */
    bool hasValue() const {
        generate();
        return !mCoroutine.done();
    }

    inline explicit operator bool () const {
        return hasValue();
    }

    T value() const {
        generate();
        return mCoroutine.promise().value();
    }

    T &&value() {
        generate();
        return std::move(mCoroutine.promise().value());
    }

    T operator *() const {
        return value();
    }

    T &&operator *() {
        return std::move(value());
    }

private:
    void generate() const {
        if (!mCoroutine.promise().hasValue()) {
            mCoroutine.resume();
            if (mCoroutine.promise().exception()) {
                std::rethrow_exception(mCoroutine.promise().exception());
            }
        }
    }

    std::coroutine_handle<promise_type> mCoroutine;
};

} // namespace QCoro

template<typename T>
QCoro::Generator<T> QCoro::detail::GeneratorPromise<T>::get_return_object() {
    return QCoro::Generator<T>(std::coroutine_handle<typename QCoro::Generator<T>::promise_type>::from_promise(*this));
}
