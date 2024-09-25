// SPDX-FileCopyrightText: 2024 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "qcorotask.h"

namespace QCoro {

template<typename T>
class LazyTask;

/*! \cond internal */

namespace detail {

//! Specialization of QCoro::detail::isTask for LazyTask.
template<typename T>
struct isTask<QCoro::LazyTask<T>> : std::true_type {
    using return_type = typename QCoro::LazyTask<T>::value_type;
};

template<typename T>
class LazyTaskPromise : public TaskPromise<T>
{
public:
    using TaskPromise<T>::TaskPromise;

    LazyTask<T> get_return_object() noexcept;

    std::suspend_always initial_suspend() const noexcept;
};

} // namespace detail

/*! \endcond */

template<typename T = void>
class LazyTask final : public detail::TaskBase<T, LazyTask, detail::LazyTaskPromise<T>>
{
public:
    using promise_type = detail::LazyTaskPromise<T>;
    using value_type = T;

    using detail::TaskBase<T, LazyTask, promise_type>::TaskBase;

    ~LazyTask() override;

    auto operator co_await() const noexcept;
};

} // namespace QCoro

#include "impl/lazytask.h"
