// SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <variant>
#include <exception>

#include "coroutine.h"

#include <QDebug>

namespace QCoro {

template<typename T>
class Generator;

namespace detail {

template<typename T>
class GeneratorPromise {
public:
    Generator<T> get_return_object();

    std::suspend_always initial_suspend() { return {}; }
    std::suspend_always final_suspend() noexcept {
        mValue = std::monostate{};
        return {};
    }

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

    T &value() {
        return std::get<T>(mValue);
    }

    bool hasValue() const {
        return std::holds_alternative<T>(mValue);
    }

    bool finished() const {
        return std::holds_alternative<std::monostate>(mValue);
    }

private:
    std::variant<std::monostate, T, std::exception_ptr> mValue;
};

template<typename T>
class GeneratorIterator {
    using promise_type = GeneratorPromise<T>;
public:
    using iterator_category = std::input_iterator_tag;
    // Not sure what type should be used for difference_type as we don't
    // allow calculating difference between two iterators.
    using difference_type = std::ptrdiff_t;
    using value_type = std::remove_reference_t<T>;
    using reference = std::add_lvalue_reference_t<T>;
    using pointer = std::add_pointer_t<value_type>;

    explicit GeneratorIterator(std::nullptr_t) {}
    explicit GeneratorIterator(std::coroutine_handle<promise_type> generatorCoroutine)
        : mGeneratorCoroutine(generatorCoroutine)
    {}

    GeneratorIterator operator++() noexcept {
        if (!mGeneratorCoroutine) {
            return *this;
        }

        mGeneratorCoroutine.resume(); // generate next value
        if (mGeneratorCoroutine.promise().finished()) {
            mGeneratorCoroutine = nullptr;
        }

        return *this;
    }

    reference operator *() const noexcept {
        if (mGeneratorCoroutine.promise().exception()) {
            std::rethrow_exception(mGeneratorCoroutine.promise().exception());
        }
        return mGeneratorCoroutine.promise().value();
    }

    bool operator==(const GeneratorIterator &other) const noexcept {
        return mGeneratorCoroutine == other.mGeneratorCoroutine;
    }

    bool operator!=(const GeneratorIterator &other) const noexcept {
        return !(operator==(other));
    }

private:
    std::coroutine_handle<promise_type> mGeneratorCoroutine{nullptr};
};

} // namespace detail

template<typename T>
class Generator {
public:
    using promise_type = detail::GeneratorPromise<T>;

    explicit Generator(std::coroutine_handle<promise_type> generatorCoroutine)
        : mGeneratorCoroutine(generatorCoroutine)
    {}

    ~Generator() {
        mGeneratorCoroutine.destroy();
    }

    detail::GeneratorIterator<T> begin() {
        if (mGeneratorCoroutine) {
            mGeneratorCoroutine.resume(); // generate first value
            if (mGeneratorCoroutine.promise().finished()) { // did not yield anything
                return detail::GeneratorIterator<T>{nullptr};
            }
            return detail::GeneratorIterator<T>{mGeneratorCoroutine};
        }

        return detail::GeneratorIterator<T>{nullptr};
    }

    detail::GeneratorIterator<T> end() {
        return detail::GeneratorIterator<T>{nullptr};
    }

private:
    std::coroutine_handle<promise_type> mGeneratorCoroutine;
};

} // namespace QCoro

template<typename T>
QCoro::Generator<T> QCoro::detail::GeneratorPromise<T>::get_return_object() {
    return QCoro::Generator<T>(std::coroutine_handle<typename QCoro::Generator<T>::promise_type>::from_promise(*this));
}
