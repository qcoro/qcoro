// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "qcoro/coroutine.h"

#include <iostream>
#include <string>

// Awaiter is a concept that provides the await_* methods below, which are used by the
// co_await expression.
// Type is Awaitable if it supports the `co_await` operator.
//
// When compiler sees a `co_await <expr>`, it first tries to obtain an Awaitable type for
// the expression result result type:
//  - first by checking if the current coroutine's promise type has `await_transform()`
//    that for given type returns an Awaitable
//  - if it does not have await_transform, it treats the result type as awaitable.
// Thus, if the current promise type doesn't have compatible `await_transform()` and the
// type itself is not Awaitable, it cannot be `co_await`ed.
//
// If the Awaitable object has `operator co_await` overload, it calls it to obtain the
// Awaiter object. Otherwise the Awaitable object is used as an Awaiter.
//
class StringAwaiter {
public:
    explicit StringAwaiter(const std::string &value) noexcept : mValue(value) {
        std::cout << "StringAwaiter constructed with value '" << value << "'." << std::endl;
    }
    ~StringAwaiter() {
        std::cout << "StringAwaiter destroyed." << std::endl;
    }

    bool await_ready() noexcept {
        std::cout << "StringAwaiter::await_ready() called." << std::endl;
        return false;
    }
    void await_suspend(std::coroutine_handle<>) noexcept {
        std::cout << "StringAwaiter::await_suspend() called." << std::endl;
    }
    std::string await_resume() noexcept {
        std::cout << "StringAwaiter::await_resume() called." << std::endl;
        return mValue;
    }

private:
    std::string mValue;
};

class StringAwaitable {
public:
    StringAwaitable(std::string str) noexcept : mStr(std::move(str)) {
        std::cout << "StringAwaitable constructored with value '" << mStr << "'." << std::endl;
    }

    ~StringAwaitable() {
        std::cout << "StringAwaitable destroyed." << std::endl;
    }

    StringAwaiter operator co_await() {
        std::cout << "StringAwaitable::operator co_await() called." << std::endl;
        return StringAwaiter{mStr};
    }

private:
    std::string mStr;
};

class VoidPromise {
public:
    explicit VoidPromise() {
        std::cout << "VoidPromise constructed." << std::endl;
    }

    ~VoidPromise() {
        std::cout << "VoidPromise destroyed." << std::endl;
    }

    struct promise_type {
        explicit promise_type() {
            std::cout << "VoidPromise::promise_type constructed." << std::endl;
        }

        ~promise_type() {
            std::cout << "VoidPromise::promise_type destroyed." << std::endl;
        }

        // Says whether the coroutine body should be executed immediately (`suspend_never`)
        // or whether it should be executed only once the coroutine is co_awaited.
        std::suspend_never initial_suspend() const noexcept {
            return {};
        }
        // Says whether the coroutine should be suspended after returning a result
        // (`suspend_always`) or whether it should just end and the frame pointer and everything
        // should be destroyed.
        std::suspend_never final_suspend() const noexcept {
            return {};
        }

        // Called by the compiler during initial coroutine setup to obtain the object that
        // will be returned from the coroutine when it is suspended.
        // Sicne this is a promise type for VoidPromise, we return a VoidPromise.
        VoidPromise get_return_object() {
            std::cout << "VoidPromise::get_return_object() called." << std::endl;
            return VoidPromise();
        }

        // Called by the compiler when an exception propagates from the coroutine.
        // Alternatively, we could declare `set_exception()` which the compiler would
        // call instead to let us handle the exception (e.g. propagate it)
        void unhandled_exception() {
            std::terminate();
        }

        // The result of the promise. Since our promise is void, we must implement `return_void()`.
        // If our promise would be returning a value, we would have to implement `return_value()`
        // instead.
        void return_void() const noexcept {};

        StringAwaitable await_transform(std::string str) {
            std::cout << "VoidPromise::await_transform for string '" << str << "' called."
                      << std::endl;
            return StringAwaitable{std::move(str)};
        }
    };
};

std::string regularFunction() {
    return "Hello World!";
}

// This function co_awaits, therefore it's a co-routine and must
// have a promise type to return to the caller.
VoidPromise myCoroutine() {
    // 1. Compiler creates a new coroutine frame `f`
    // 2. Compiler obtains a return object from the promise.
    //    The promise is of type `std::coroutine_traits<CurrentFunctionReturnType>::promise_type`,
    //    which is `CurrentFunctionReturnType::promise_type` (if there is no specialization for
    //    `std::coroutine_traits<CurrentFunctionReturnType>`)
    // 3. Compiler starts execution of the coroutine body by calling `resume()` on the
    //    current coroutine's std::coroutine_handle (obtained from the promise by
    //    `std::coroutine_handle<decltype(f->promise)>::from_promise(f->promise)

    std::cout << "myCoroutine() started." << std::endl;
    const auto result = co_await regularFunction();
    std::cout << "Result successfully co_await-ed: " << result << std::endl;
}

int main() {
    std::cout << "Calling myCoroutine() from main()." << std::endl;
    myCoroutine();
    std::cout << "Returned from myCoroutine() to main()." << std::endl;

    return 0;
}
