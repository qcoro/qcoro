// SPDX-FileCopyrightText: 2023 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "coroutine.h"

#include <QPointer>

#include <optional>

class QObject;

namespace QCoro {
// \cond internal
namespace detail {
class TaskPromiseBase;
struct ThisCoroPromise;
} // namespace detail
// \endcond

auto thisCoro() -> detail::ThisCoroPromise;

// \cond internal
namespace detail {
struct ThisCoroPromise
{
private:
    ThisCoroPromise() = default;
    friend ThisCoroPromise QCoro::thisCoro();
};
} // namespace detail

// \endcond

//! Returns current coroutine's features when co_awaited.
/*!
 * Use `co_await QCoro::thisCoro() inside a coroutine to obtain its Features
 * configuration object.
 */
inline auto thisCoro() -> detail::ThisCoroPromise
{
    return detail::ThisCoroPromise{};
}

//! Features of current coroutines
/*!
 * Allows configuring behavior of the current coroutine.
 *
 * Use `co_await QCoro::thisCoro()` to obtain the current coroutine's Features
 * and modify them.
 */
class CoroutineFeatures
{
public:
    constexpr bool await_ready() noexcept
    {
        return true;
    }
    // Could be &&, but that fails on GCC 10 due to handle being passed as an lvalue.
    constexpr void await_suspend(std::coroutine_handle<>) noexcept {}

    constexpr CoroutineFeatures & await_resume() noexcept
    {
        return *this;
    }

    //! Bind the coroutine lifetime to the lifetime of the given object
    /*!
     * Watches the given \c obj object. If the object is destroyed while the
     * current coroutine is suspended, the coroutine will be destroyed immediately
     * after resuming to prevent a use-after-free "this" pointer.
     *
     * \param obj QObject to observe its lifetime. Pass `nullptr` to stop observing.
     */
    void guardThis(QObject *obj)
    {
        if (!obj) {
            mGuardedThis.reset();
        } else {
            mGuardedThis.emplace(obj);
        }
    }

    auto guardedThis() const -> std::optional<QPointer<QObject>>
    {
        return mGuardedThis;
    }

private:
    std::optional<QPointer<QObject>> mGuardedThis;

    explicit CoroutineFeatures() = default;
    friend class QCoro::detail::TaskPromiseBase;
};

} // namespace QCoro






