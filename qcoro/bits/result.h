// SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "cancellation.h"

#include <variant>

namespace QCoro {

namespace detail {
template<typename T>
class CancellableTaskPromise;
} // namespace detail

template<typename T>
class Result {
public:
    Result(const Result &) = delete;
    Result &operator=(const Result &) = delete;
    Result(Result &&) noexcept = default;
    Result &operator=(Result &&) noexcept = default;
    ~Result() = default;

    bool isCancelled() const {
        return std::holds_alternative<detail::Cancellation_t>(mResult);
    }

        operator bool() const { // NOLINT google-explicit-conversion
        return !isCancelled();
    }

    T &&operator*() {
        Q_ASSERT(!isCancelled());
        return std::move(std::get<T>(mResult));
    }

    const T &operator*() const {
        Q_ASSERT(!isCancelled());
        return std::get<T>(mResult);
    }

    T &&value() {
        Q_ASSERT(!isCancelled());
        return std::move(std::get<T>(mResult));
    }

    const T &value() const {
        Q_ASSERT(!isCancelled());
        return std::get<T>(mResult);
    }

private:
    friend class detail::CancellableTaskPromise<T>;

    explicit Result(detail::Cancellation_t cancellation)
        : mResult(cancellation) {}
    explicit Result(T &&result)
        : mResult(std::forward<T>(result)) {}
    explicit Result(const T &result)
        : mResult(result) {}

    std::variant<detail::Cancellation_t, T> mResult;
};

template<>
class Result<void> {
public:
    Result(const Result &) = delete;
    Result &operator=(const Result &) = delete;
    Result(Result &&) noexcept = default;
    Result &operator=(Result &&) noexcept = default;
    ~Result() = default;

    bool isCancelled() const {
        return mCancelled;
    }

    operator bool() const { // NOLINT google-explicit-conversion
        return !isCancelled();
    }

private:
    friend class detail::CancellableTaskPromise<void>;

    explicit Result() = default;
    explicit Result(detail::Cancellation_t)
        : mCancelled(true) {}

    bool mCancelled = false;
};


} // namespace QCoro
