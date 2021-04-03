// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "task.h"
#include "qcoroprocess.h"

namespace QCoro {

//! Returns a coroutine-friendly wrapper for QProcess object.
/*!
 * Returns a wrapper for the QProcess \c p that provides coroutine-friendly
 * versions of certain async APIs.
 */
inline auto coro(QProcess &p) noexcept { return detail::QCoroProcess{&p}; }
//! \copydoc QCoro::coro(QProcess &p) noexcept
inline auto coro(QProcess *p) noexcept { return detail::QCoroProcess{p}; }

} // namespace QCoro


