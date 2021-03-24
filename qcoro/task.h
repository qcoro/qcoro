#pragma once

#include "coroutine.h"

#include <variant>

namespace QCoro
{

template<typename T = void>
class Task;

namespace detail {

template<typename T>
struct awaiter_type;

template<typename T>
using awaiter_type_t = typename awaiter_type<T>::type;


class TaskPromiseBase {
public:
    constexpr QCORO_STD::suspend_never initial_suspend() const noexcept { return {}; }
    constexpr QCORO_STD::suspend_never final_suspend() const noexcept { return {}; }

    template<typename T,
             typename Awaiter = QCoro::detail::awaiter_type_t<T>>
    auto await_transform(T &&value) {
        return Awaiter{value};
    }

};

template<typename T>
class TaskPromise: public TaskPromiseBase {
public:
    Task<T> get_return_object() const;

    void unhandled_exception() {
        mValue.emplace(std::current_exception());
    }

    void return_value(T &&value) noexcept {
        mValue.emplace(std::forward<T>(value));
    }

    T &result() & {
        if (std::holds_alternative<std::exception_ptr>(mValue)) {
            std::rethrow_exception(std::get<std::exception_ptr>(mValue));
        }

        return std::get<T>(mValue);
    }

    T &&result() && {
        if (std::holds_alternative<std::exception_ptr>(mValue)) {
            std::rethrow_exception(std::get<std::exception_ptr>(mValue));
        }

        return std::move(std::get<T>(mValue));
    }

private:
    std::variant<std::monostate, T, std::exception_ptr> mValue;
};

template<>
class TaskPromise<void> : public TaskPromiseBase {
public:
    Task<void> get_return_object() const;

    void unhandled_exception() {
        mException = std::current_exception();
    }

    void return_void() noexcept {};

    void result() {
        if (mException) {
            std::rethrow_exception(mException);
        }
    }

private:
    std::exception_ptr mException;
};


} // namespace detail

// Promise
template<typename T>
class Task {
public:
    using promise_type = detail::TaskPromise<T>;
    using value_type = T;
};

template<>
class Task<void> {
public:
    using promise_type = detail::TaskPromise<void>;
    using value_type = void;
};


namespace detail {

template<typename T>
Task<T> TaskPromise<T>::get_return_object() const {
    return Task<T>{};
}

Task<void> TaskPromise<void>::get_return_object() const {
    return Task<void>{};
}

} // namespace detail

} // namespace QCoro

