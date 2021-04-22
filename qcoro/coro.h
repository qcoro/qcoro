// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "task.h"
#include "qcoroprocess.h"
#include "qcoroiodevice.h"
#include "qcorolocalsocket.h"
#include "qcoroabstractsocket.h"
#include "qcoronetworkreply.h"
#include "qcorosignal.h"

//! Allows co_awaiting on signal emission.
/*!
 * Returns an Awaitable object that allows co_awaiting for a signal to
 * be emitted. The result of the co_awaiting is a tuple with the signal
 * arguments.
 *
 * @see docs/reference/coro.md
 */
template<QCoro::detail::concepts::QObject T, typename FuncPtr>
inline auto qCoro(T *obj, FuncPtr &&ptr) { return QCoro::detail::QCoroSignal(obj, std::forward<FuncPtr>(ptr)); }

//! Returns a coroutine-friendly wrapper for QProcess object.
/*!
 * Returns a wrapper for the QProcess \c p that provides coroutine-friendly
 * way to co_await the process to start or finish.
 *
 * @see docs/reference/qprocess.md
 */
inline auto qCoro(QProcess &p) noexcept { return QCoro::detail::QCoroProcess{&p}; }
//! \copydoc qCoro(QProcess &p) noexcept
inline auto qCoro(QProcess *p) noexcept { return QCoro::detail::QCoroProcess{p}; }

//! Returns a coroutine-friendly wrapper for QLocalSocket object.
/*!
 * Returns a wrapper for the QLocalSocket \c s that provides coroutine-friendly
 * way to co_await the socket to connect and disconnect.
 *
 * @see docs/reference/qlocalsocket.md
 */
inline auto qCoro(QLocalSocket &s) noexcept { return QCoro::detail::QCoroLocalSocket{&s}; }
//! \copydoc qCoro(QLocalSocket &s) noexcept
inline auto qCoro(QLocalSocket *s) noexcept { return QCoro::detail::QCoroLocalSocket{s}; }

//! Returns a coroutine-friendly wrapper for QAbstractSocket object.
/*!
 * Returns a wrapper for the QAbstractSocket \c s that provides coroutine-friendly
 * way to co_await the socket to connect and disconnect.
 *
 * @see docs/reference/qabstractsocket.md
 */
inline auto qCoro(QAbstractSocket &s) noexcept { return QCoro::detail::QCoroAbstractSocket{&s}; }
//! \copydoc qCoro(QAbstractSocket &s) noexcept
inline auto qCoro(QAbstractSocket *s) noexcept { return QCoro::detail::QCoroAbstractSocket{s}; }

//! Returns a coroutine-friendly wrapper for QNetworkReply object.
/*!
 * Returns a wrapper for the QNetworkReply \c s that provides coroutine-friendly
 * way to co_await read and write operations.
 *
 * @see docs/reference/qnetworkreply.md
 */
inline auto qCoro(QNetworkReply &s) noexcept { return QCoro::detail::QCoroNetworkReply{&s}; }
//! \copydoc qCoro(QAbstractSocket &s) noexcept
inline auto qCoro(QNetworkReply *s) noexcept { return QCoro::detail::QCoroNetworkReply{s}; }

//! Returns a coroutine-friendly wrapper for a QIODevice-derived object.
/*!
 * Returns a wrapper for QIODevice \c d that provides coroutine-friendly way
 * of co_awaiting reading and writing operation.
 *
 * @see docs/reference/qiodevice.md
 */
inline auto qCoro(QIODevice &d) noexcept { return QCoro::detail::QCoroIODevice{&d}; }
//! \copydoc qCoro(QIODevice *d) noexcept
inline auto qCoro(QIODevice *d) noexcept { return QCoro::detail::QCoroIODevice{d}; }


