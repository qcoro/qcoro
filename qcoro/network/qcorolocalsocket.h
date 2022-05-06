// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "waitoperationbase_p.h"
#include "qcoroiodevice.h"
#include "qcoronetwork_export.h"

#include <QLocalSocket>

#include <chrono>

namespace QCoro::detail {

using namespace std::chrono_literals;

//! QLocalSocket wrapper with co_awaitable-friendly API.
class QCORONETWORK_EXPORT QCoroLocalSocket : public QCoroIODevice {
public:
    explicit QCoroLocalSocket(QLocalSocket *socket);

    /**
     * \brief Co_awaitable equivalent  to [`QLocalSocket::waitForConnected()`][qtdoc-qlocalsocket-waitForConnected].
     *
     * Waits for at most \c timeout_msecs milliseconds. If the timeout is -1, the call will never timeout.
     *
     * \return Returns `true` when successfully connected, `false` is an error occured or the connection
     * wasn't established within the given timeout.
     *
     * [qtdoc-qlocalsocket-waitForConnected]: https://doc.qt.io/qt-5/qlocalsocket.html#waitForConnected
     */
    Task<bool> waitForConnected(int timeout_msecs = 30'000);

    /**
     * \brief Co_awaitable equivalent to [`QLocalSocket::waitForConnected()`][qtdoc-qlocalsocket-waitForConnected].
     *
     * Unlike the Qt version, this overload uses `std::chrono::milliseconds` to express the
     * timeout rather than plain `int`. If the \c timeout is -1, the call will never time out.
     *
     * \return Returns `true` when successfully connected, `false` is an error occured or the connection
     * wasn't established within the given timeout.
     *
     * [qtdoc-qlocalsocket-waitForConnected]: https://doc.qt.io/qt-5/qlocalsocket.html#waitForConnected
     */
    Task<bool> waitForConnected(std::chrono::milliseconds timeout);

    /**
     * \brief Co_awaitable equivalent to [`QLocalSocket::waitForDisconnected()`][qtdoc-qlocalsocket-waitForDisconnected].
     *
     * Waits for at most \c timeout_msecds milliseconds. If the timeout is -1, the call will never time out.
     *
     * \returns Returns `true` when the socket has been disconnected successfully. If the socket
     * wasn't connected, or doesn't disconnected within the specified timeout, the coroutine returns
     * `false`.
     *
     * [qtdoc-qlocalsocket-waitForDisconnected]: https://doc.qt.io/qt-5/qlocalsocket.hmtl#waitForDisconnected
     */
    Task<bool> waitForDisconnected(int timeout_msecs = 30'000);

    /**
     * \brief Co_awaitable equivalent to [`QLocalSocket::waitForDisconnected()`][qtdoc-qlocalsocket-waitForDisconnected].
     *
     * Unlike the Qt version, this overload uses `std::chrono::milliseconds` to express the
     * timeout rather than plain `int`. If the \c timeout is -1, the call will never time out.
     *
     * \returns Returns `true` when the socket has been disconnected successfully. If the socket
     * wasn't connected, or doesn't disconnected within the specified timeout, the coroutine returns
     * `false`.
     *
     * [qtdoc-qlocalsocket-waitForDisconnected]: https://doc.qt.io/qt-5/qlocalsocket.hmtl#waitForDisconnected
     */
    Task<bool> waitForDisconnected(std::chrono::milliseconds timeout);

    /**
     * \brief Connects to server and waits until the connection is established.
     *
     * Equivalent to calling [`QLocalSocket::connecToServer`][qdoc-qlocalsocket-connecToServer]
     * followed by [`QLocalSocket::waitForConnected`][qdoc-qlocalsocket-waitForConnected].
     *
     * Waits for at most \c timeout milliseconds. If the \c timeout is -1, the call will never time out.
     *
     * \return Returns `true` when connection is successfully established, `false` otherwise.
     *
     * [qtdoc-qlocalsocket-connectToServer]: https://doc.qt.io/qt-5/qlocalsocket.html#connectToServer
     * [qtdoc-qlocalsocket-waitForConnected]: https://doc.qt.io/qt-5/qlocalsocket.html#waitForConnected
     */
    Task<bool> connectToServer(QIODevice::OpenMode openMode = QIODevice::ReadWrite,
                               std::chrono::milliseconds timeout = std::chrono::seconds{30});

    /**
     * \brief Connects to server and waits until the connection is established.
     *
     * Equivalent to calling [`QLocalSocket::connecToServer`][qdoc-qlocalsocket-connecToServer]
     * followed by [`QLocalSocket::waitForConnected`][qdoc-qlocalsocket-waitForConnected].
     *
     * Waits for at most \c timeout milliseconds. If the \c timeout is -1, the call will never time out.
     *
     * \returns Returns `true` when connection is successfully established, `false` otherwise.
     *
     * [qtdoc-qlocalsocket-connectToServer-1]: https://doc.qt.io/qt-5/qlocalsocket.html#connectToServer-1
     * [qtdoc-qlocalsocket-waitForConnected]: https://doc.qt.io/qt-5/qlocalsocket.html#waitForConnected
     */
    Task<bool> connectToServer(const QString &name, QIODevice::OpenMode openMode = QIODevice::ReadWrite,
                               std::chrono::milliseconds timeout = std::chrono::seconds{30});

private:
    Task<std::optional<bool>> waitForReadyReadImpl(std::chrono::milliseconds timeout) override;
    Task<std::optional<qint64>> waitForBytesWrittenImpl(std::chrono::milliseconds timeout) override;
};

} // namespace QCoro::detail

//! Returns a coroutine-friendly wrapper for QLocalSocket object.
/*!
 * Returns a wrapper for the QLocalSocket \c s that provides coroutine-friendly
 * way to co_await the socket to connect and disconnect.
 *
 * @see docs/reference/qlocalsocket.md
 */
inline auto qCoro(QLocalSocket &s) noexcept {
    return QCoro::detail::QCoroLocalSocket{&s};
}
//! \copydoc qCoro(QLocalSocket &s) noexcept
inline auto qCoro(QLocalSocket *s) noexcept {
    return QCoro::detail::QCoroLocalSocket{s};
}

