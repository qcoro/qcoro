// SPDX-FileCopyrightText: 2023 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

/*
 * Do NOT include this file directly - include the QCoroTask header instead!
 */

#pragma once

#include "../qcorotask.h"

namespace QCoro::detail
{

inline TaskFinalSuspend::TaskFinalSuspend(std::vector<CoroutineHandle> awaitingCoroutines)
    : mAwaitingCoroutines(std::move(awaitingCoroutines)) {}

inline bool TaskFinalSuspend::await_ready() const noexcept {
    return false;
}

template<typename Promise>
inline void TaskFinalSuspend::await_suspend(std::coroutine_handle<Promise> finishedCoroutine) noexcept {
    auto &promise = finishedCoroutine.promise();

    for (auto &awaiter : mAwaitingCoroutines) {
        if (const auto qcoro_handle = std::get_if<std::coroutine_handle<TaskPromiseBase>>(&awaiter); qcoro_handle != nullptr) {
            auto &promise = qcoro_handle->promise();
            if (const CoroutineFeatures *features = promise.features(); features != nullptr) {
                if (const auto &guardedThis = features->guardedThis(); guardedThis.has_value() && guardedThis->isNull()) {
                    // We have a QPointer, but it's null which means that observed QObject has been destroyed,
                    // so just destroy the current coroutine as well.
                    promise.destroyCoroutine(true);
                    continue;
                }
            }

            qcoro_handle->resume();
        } else {
            std::get<std::coroutine_handle<>>(awaiter).resume();
        }
    }
    mAwaitingCoroutines.clear();

    // The handle will be destroyed here only if the associated Task has already been destroyed
    promise.derefCoroutine();
}

constexpr void TaskFinalSuspend::await_resume() const noexcept {}

} // namespace QCoro::detail
