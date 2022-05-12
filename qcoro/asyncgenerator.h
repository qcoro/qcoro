// SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT
//
// Based on cppcoro::async_generator (c) Lewis Baker, published under MIT license
// https://github.com/lewissbaker/cppcoro/blob/master/include/cppcoro/async_generator.hpp

#pragma once

#include "coroutine.h"

#include <iterator>
#include <exception>

namespace QCoro {

template<typename T>
class AsyncGenerator;

namespace detail
{

template<typename T>
class AsyncGeneratorIterator;
class AsyncGeneratorYieldOperation;
class AsyncGeneratorAdvanceOperation;

class AsyncGeneratorPromiseBase {
public:
    AsyncGeneratorPromiseBase() noexcept = default;
    AsyncGeneratorPromiseBase(const AsyncGeneratorPromiseBase &) = delete;
    AsyncGeneratorPromiseBase& operator=(const AsyncGeneratorPromiseBase &) = delete;

    std::suspend_always initial_suspend() const noexcept { return {}; }

    AsyncGeneratorYieldOperation final_suspend() noexcept;

    void unhandled_exception() noexcept {
        m_exception = std::current_exception();
    }

    void return_void() noexcept {}

    /// Query if the generator has reached the end of the sequence.
    ///
    /// Only valid to call after resuming from an awaited advance operation.
    /// i.e. Either a begin() or iterator::operator++() operation.
    bool finished() const noexcept {
        return m_currentValue == nullptr;
    }

    void rethrow_if_unhandled_exception() {
        if (m_exception) {
            std::rethrow_exception(std::move(m_exception));
        }
    }

protected:
    AsyncGeneratorYieldOperation internal_yield_value() noexcept;

private:
    friend class AsyncGeneratorYieldOperation;
    friend class AsyncGeneratorAdvanceOperation;
    friend class IteratorAwaitableBase;

    std::exception_ptr m_exception = nullptr;
    std::coroutine_handle<> m_consumerCoroutine;

protected:
    void *m_currentValue = nullptr;
};

class AsyncGeneratorYieldOperation final {
public:
    AsyncGeneratorYieldOperation(std::coroutine_handle<> consumer) noexcept
        : m_consumer(consumer)
    {}

    bool await_ready() const noexcept { return false; }

    std::coroutine_handle<> await_suspend([[maybe_unused]] std::coroutine_handle<> producer) noexcept {
        return m_consumer;
    }

    void await_resume() noexcept {}

private:
    std::coroutine_handle<> m_consumer;
};

inline AsyncGeneratorYieldOperation AsyncGeneratorPromiseBase::final_suspend() noexcept {
    m_currentValue = nullptr;
    return internal_yield_value();
}

inline AsyncGeneratorYieldOperation AsyncGeneratorPromiseBase::internal_yield_value() noexcept {
    return AsyncGeneratorYieldOperation{m_consumerCoroutine};
}

class IteratorAwaitableBase {
protected:
    IteratorAwaitableBase(std::nullptr_t) noexcept {}
    IteratorAwaitableBase(
        AsyncGeneratorPromiseBase &promise,
        std::coroutine_handle<> producerCoroutine) noexcept
        : m_promise(std::addressof(promise))
        , m_producerCoroutine(producerCoroutine) {}

public:
    bool await_ready() const noexcept { return false; }

    std::coroutine_handle<> await_suspend(std::coroutine_handle<> consumerCoroutine) noexcept {
        m_promise->m_consumerCoroutine = consumerCoroutine;
        return m_producerCoroutine;
    }

protected:
    AsyncGeneratorPromiseBase *m_promise = nullptr;
    std::coroutine_handle<> m_producerCoroutine = {nullptr};
};

template<typename T>
class AsyncGeneratorPromise final : public AsyncGeneratorPromiseBase {
    using value_type = std::remove_reference_t<T>;
public:
    AsyncGeneratorPromise() noexcept = default;

    AsyncGenerator<T> get_return_object() noexcept;

    AsyncGeneratorYieldOperation yield_value(value_type &value) noexcept {
        m_currentValue = std::addressof(value);
        return internal_yield_value();
    }

    AsyncGeneratorYieldOperation yield_value(value_type &&value) noexcept {
        return yield_value(value);
    }

    T &value() const noexcept {
        return *static_cast<T *>(m_currentValue);
    }
};

template<typename T>
class AsyncGeneratorPromise<T &&> final : public AsyncGeneratorPromiseBase {
public:
    AsyncGeneratorPromise() noexcept = default;
    AsyncGenerator<T> get_return_object() noexcept;

    AsyncGeneratorYieldOperation yield_value(T &&value) noexcept {
        m_currentValue = std::addressof(value);
        return internal_yield_value();
    }

    T &&value() const noexcept {
        return std::move(*static_cast<T *>(m_currentValue));
    }
};

template<typename T>
class AsyncGeneratorIterator final
{
    using promise_type = AsyncGeneratorPromise<T>;
    using handle_type = std::coroutine_handle<promise_type>;
public:
    using iterator_category = std::input_iterator_tag;
    // Not sure what type should be used for difference_type as we don't
    // allow calculating difference between two iterators.
    using difference_type = std::ptrdiff_t;
    using value_type = std::remove_reference_t<T>;
    using reference = std::add_lvalue_reference_t<T>;
    using pointer = std::add_pointer_t<value_type>;

    AsyncGeneratorIterator(std::nullptr_t) noexcept {}
    AsyncGeneratorIterator(handle_type coroutine) noexcept
        : m_coroutine(coroutine)
    {}

    auto operator++() noexcept {
        class IncrementIteratorAwaitable final : public IteratorAwaitableBase {
        public:
            IncrementIteratorAwaitable(AsyncGeneratorIterator &iterator) noexcept
                : IteratorAwaitableBase(iterator.m_coroutine.promise(), iterator.m_coroutine)
                , m_iterator(iterator)
            {}

            AsyncGeneratorIterator<T> &await_resume() {
                if (m_promise->finished()) {
                    m_iterator = AsyncGeneratorIterator<T>{nullptr};
                    m_promise->rethrow_if_unhandled_exception();
                }
                return m_iterator;
            }
        private:
            AsyncGeneratorIterator<T> &m_iterator;
        };

        return IncrementIteratorAwaitable{*this};
    }

    reference operator *() const noexcept {
        return m_coroutine.promise().value();
    }

    bool operator==(const AsyncGeneratorIterator &other) const noexcept {
        return m_coroutine == other.m_coroutine;
    }

    bool operator!=(const AsyncGeneratorIterator &other) const noexcept {
        return !(*this == other);
    }

private:
    handle_type m_coroutine = {nullptr};
};

} // namespace detail

/**
 * @brief AsyncGenerator<T> is a return type for a generator coroutine.
 *
 * Generator is a function that yields a value and suspends its execution
 * until another value is requested by the caller. The AsyncGenerator<T>
 * allows the generator function to also contain `co_await`.
 *
 * The generator behaves like an iterable, so one can easilly iterate over
 * values produced by the generator, until the generator finishes. If the
 * AsyncGenerator<T> object is destroyed while the generator coroutine hasn't
 * finished yet, the coroutine will be suspended and terminated.
 *
 * The generator function is resumed whenever the iterator is incremented.
 * Therefore, the increment operation (operator++()) must be co_awaited.
 * Retrieveing the generated value from the current iterator is a synchronous
 * operation.
 *
 * @code
 * // Asynchronously generates "count" random numbers
 * QCoro::AsyncGenerator<int> randomValueGenerator(int count) {
 *   for (int i = 0; i < count; ++i) {
 *     co_yield co_await generateRandomNumber();
 *   }
 * }
 *
 * QCoro::Task<int> randomSum(int count) {
 *   int sum = 0;
 *   QCORO_FOREACH(int rand, randomValueGenerator(10)) {
 *     sum += rand;
 *   }
 *   return sum;
 * }
 * @endcode
 **/
template<typename T>
class [[nodiscard]] AsyncGenerator {
public:
    using promise_type = detail::AsyncGeneratorPromise<T>;
    using iterator = detail::AsyncGeneratorIterator<T>;

    /// Constructs an empty asynchronous generator
    AsyncGenerator() noexcept
        : m_coroutine(nullptr) {}

    /// Constructs AsyncGenerator for given promise.
    explicit AsyncGenerator(promise_type &promise) noexcept
        : m_coroutine(std::coroutine_handle<promise_type>::from_promise(promise))
    {}

    /// Move constructor
    AsyncGenerator(AsyncGenerator &&other) noexcept
        : m_coroutine(other.m_coroutine) {
        other.m_coroutine = nullptr;
    }

    /// Destructor.
    ~AsyncGenerator() {
        if (m_coroutine) {
            m_coroutine.destroy();
        }
    }

    /// Move-assignment operator.
    AsyncGenerator& operator=(AsyncGenerator &&other) noexcept {
        AsyncGenerator temp(std::move(other));
        swap(temp);
        return *this;
    }

    /// Copy constructor
    AsyncGenerator(const AsyncGenerator &) = delete;
    /// Copy assignment operator
    AsyncGenerator& operator=(const AsyncGenerator &) = delete;

    /**
     * \brief Returns an iterator-like awaitable
     *
     * The returned object is an awaitable that must be co_awaited to obtain
     * an asynchronous iterator to iterate over results of the asychronous generator.
     *
     * The asynchronout iterator can be used like a regular iterator, except that incrementing
     * the iterator returns an awaitable that must be co_awaited.
     */
    auto begin() noexcept {
        class BeginIteratorAwaitable final : public detail::IteratorAwaitableBase {
        public:
            BeginIteratorAwaitable(std::nullptr_t) noexcept
                : IteratorAwaitableBase(nullptr) {}
            BeginIteratorAwaitable(std::coroutine_handle<promise_type> producerCoroutine) noexcept
                : IteratorAwaitableBase(producerCoroutine.promise(), producerCoroutine) {}

            bool await_ready() const noexcept {
                return m_promise == nullptr || IteratorAwaitableBase::await_ready();
            }

            detail::AsyncGeneratorIterator<T> await_resume() {
                if (m_promise == nullptr) {
                    return detail::AsyncGeneratorIterator<T>{nullptr};
                } else if (m_promise->finished()) {
                    m_promise->rethrow_if_unhandled_exception();
                    return detail::AsyncGeneratorIterator<T>{nullptr};
                }
                return detail::AsyncGeneratorIterator<T>{
                    std::coroutine_handle<promise_type>::from_promise(*static_cast<promise_type *>(m_promise))
                };
            }
        };

        return m_coroutine ? BeginIteratorAwaitable{m_coroutine} : BeginIteratorAwaitable{nullptr};
    }

    /// Returns an iterator representing the finished generator.
    auto end() noexcept {
        return iterator{nullptr};
    }

    void swap(AsyncGenerator &other) noexcept {
        using std::swap;
        swap(m_coroutine, other.m_coroutine);
    }

private:
    std::coroutine_handle<promise_type> m_coroutine;
};

template<typename T>
void swap(AsyncGenerator<T> &a, AsyncGenerator<T> &b) noexcept {
    a.swap(b);
}

namespace detail {

template<typename T>
AsyncGenerator<T> AsyncGeneratorPromise<T>::get_return_object() noexcept {
    return AsyncGenerator<T>{*this};
}

} // namespace detail

} // namespace QCoro

/**
 * \brief Helper macro to asynchronously loop over values produced by the AsyncGenerator<T>.
 *
 * The macro is identical to Q_FOREACH, except that it makes sure to `co_await` on each
 * iteration. It's identical to the following piece of code:
 *
 * @code
 * for (auto it = co_await generator.begin(), end = generator.end(); it != end; co_await ++it) {
 *    var = *it;
 *    ...
 * }
 * @endcode
 *
 * @param var The full declaration of the iteration variable (incl. type, cvref), e.g. "const QString &user"
 * @param generator The AsyncGenerator<T> object.
 **/
#define QCORO_FOREACH(var, generator) \
    if (auto && _container = generator; false) {} else \
        for (auto _begin = co_await _container.begin(), _end = _container.end(); \
             _begin != _end; \
             co_await ++_begin) \
         if (var = *_begin; false) {} else
