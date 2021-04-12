// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "task.h"
#include "qcoroprocess.h"
#include "qcoroiodevice.h"
#include "qcorolocalsocket.h"

namespace QCoro {

//! Returns a coroutine-friendly wrapper for QProcess object.
/*!
 * Returns a wrapper for the QProcess \c p that provides coroutine-friendly
 * way to co_await the process to start or finish.
 */
inline auto coro(QProcess &p) noexcept { return detail::QCoroProcess{&p}; }
//! \copydoc QCoro::coro(QProcess &p) noexcept
inline auto coro(QProcess *p) noexcept { return detail::QCoroProcess{p}; }
//
//! Returns a coroutine-friendly wrapper for QLocalSocket object.
/*!
 * Returns a wrapper for the QLocalSocket \c s that provides coroutine-friendly
 * way to co_await the socket to connect and disconnect.
 */
inline auto coro(QLocalSocket &s) noexcept { return detail::QCoroLocalSocket{&s}; }
//! \copydoc QCoro::coro(QLocalSocket &s) noexcept
inline auto coro(QLocalSocket *s) noexcept { return detail::QCoroLocalSocket{s}; }



//! Returns a coroutine-friendly wrapper for a QIODevice-derived object.
/*!
 * Returns a wrapper for QIODevice \c d that provides coroutine-friendly way
 * of co_awaiting reading and writing operation.
 */
inline auto coro(QIODevice &d) noexcept { return detail::QCoroIODevice{&d}; }
//! \copydoc QCoro::coro(QIODevice *d) noexcept
inline auto coro(QIODevice *d) noexcept { return detail::QCoroIODevice{d}; }

} // namespace QCoro


