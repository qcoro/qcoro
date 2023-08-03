// SPDX-FileCopyrightText: 2023 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

/*
 * Do NOT include this file directly - include the QCoroTask header instead!
 */

#pragma once

#include "../qcorotask.h"

#include <QPointer>

namespace QCoro
{

template <typename T, typename QObjectSubclass, typename Callback>
requires std::is_invocable_v<Callback> || std::is_invocable_v<Callback, T> || std::is_invocable_v<Callback, QObjectSubclass *> || std::is_invocable_v<Callback, QObjectSubclass *, T>
inline void connect(QCoro::Task<T> &&task, QObjectSubclass *context, Callback func) {
    QPointer ctxWatcher = context;
    if constexpr (std::is_same_v<T, void>) {
        task.then([ctxWatcher, func = std::move(func)]() {
            if (ctxWatcher) {
                if constexpr (std::is_member_function_pointer_v<Callback>) {
                    (ctxWatcher->*func)();
                } else {
                    func();
                }
            }
        });
    } else {
        task.then([ctxWatcher, func = std::move(func)](auto &&value) {
            if (ctxWatcher) {
                if constexpr (std::is_invocable_v<Callback, QObjectSubclass, T>) {
                    (ctxWatcher->*func)(std::forward<decltype(value)>(value));
                } else if constexpr (std::is_invocable_v<Callback, T>) {
                    func(std::forward<decltype(value)>(value));
                } else {
                    Q_UNUSED(value);
                    if constexpr (std::is_member_function_pointer_v<Callback>) {
                        (ctxWatcher->*func)();
                    } else {
                        func();
                    }
                }
            }
        });
    }
}

template <typename T, typename QObjectSubclass, typename Callback>
requires detail::TaskConvertible<T>
        && (std::is_invocable_v<Callback> || std::is_invocable_v<Callback, detail::convertible_awaitable_return_type_t<T>> || std::is_invocable_v<Callback, QObjectSubclass *> || std::is_invocable_v<Callback, QObjectSubclass *, detail::convertible_awaitable_return_type_t<T>>)
        && (!detail::isTask_v<T>)
inline void connect(T &&future, QObjectSubclass *context, Callback func) {
    auto task = detail::toTask(std::move(future));
    connect(std::move(task), context, func);
}

} // namespace QCoro