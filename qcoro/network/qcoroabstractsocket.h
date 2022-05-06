// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "waitoperationbase_p.h"
#include "qcoroiodevice.h"
#include "qcoronetwork_export.h"

#include <QPointer>
#include <QAbstractSocket>

#include <chrono>

class QAbstractSocket;

namespace QCoro::detail {

using namespace std::chrono_literals;

//! QAbstractSocket wrapper with co_awaitable-friendly API.
class QCORONETWORK_EXPORT QCoroAbstractSocket final : public QCoroIODevice {
public:
    explicit QCoroAbstractSocket(QAbstractSocket *socket);

    //! Co_awaitable equivalent  to [`QAbstractSocket::waitForConnected()`][qtdoc-qabstractsocket-waitForConnected].
    /*!
     * Waits until the socket is connected, up to \c timeout_msecs milliseconds. If the connection has been
     * established, the coroutine returns `true`, otherwise it retuns `false`.
     *
     * If the timeout is -1, the operation will never time out.
     *
     * [qtdoc-qabstractsocket-waitForConnected]: https://doc.qt.io/qt-5/qabstractsocket.html#waitForConnected
     */
    Task<bool> waitForConnected(int timeout_msecs = 30'000);

    //! Co_awaitable equivalent to [`QAbstractSocket::waitForConnected()`][qtdoc-qabstractsocket-waitForConnected].
    /*!
     * Waits until the socket is connected, up to \c timeout_msecs milliseconds. If the connection has been
     * established, the coroutine returns `true`, otherwise it retuns `false`.
     *
     * Unlike the Qt version, this overload uses `std::chrono::milliseconds` to express the
     * timeout rather than plain `int`. If the timeout is -1, the operation will never time out.
     *
     *
     * [qtdoc-qabstractsocket-waitForConnected]: https://doc.qt.io/qt-5/qabstractsocket.html#waitForConnected
     */
    Task<bool> waitForConnected(std::chrono::milliseconds timeout);

    //! Co_awaitable equivalent to [`QAbstractSocket::waitForDisconnected()`][qtdoc-qabstractsocket-waitForDisconnected].
    /*!
     * Waits until the socket has disconnected, up to \c timeout_msecs milliseconds. If the connection was
     * successfully disconnected, returns `true`, otherwise returns `false` (if the operation timed out,
     * if an error occurred or if the `QAbstractSocket` is already disconnected).
     *
     * If the timeout is -1, the operation will never time out.
     *
     * [qtdoc-qabstractsocket-waitForDisconnected]: https://doc.qt.io/qt-5/qabstractsocket.hmtl#waitForDisconnected
     */
    Task<bool> waitForDisconnected(int timeout_msecs = 30'000);

    //! Co_awaitable equivalent to [`QAbstractSocket::waitForDisconnected()`][qtdoc-qabstractsocket-waitForDisconnected].
    /*!
     * Waits until the socket has disconnected, up to \c timeout_msecs milliseconds. If the connection was
     * successfully disconnected, returns `true`, otherwise returns `false` (if the operation timed out,
     * if an error occurred or if the `QAbstractSocket` is already disconnected).
     *
     * Unlike the Qt version, this overload uses `std::chrono::milliseconds` to express the
     * timeout rather than plain `int`. If the timeout is -1, the operation will never time out.
     *
     * [qtdoc-qabstractsocket-waitForDisconnected]: https://doc.qt.io/qt-5/qabstractsocket.hmtl#waitForDisconnected
     */
    Task<bool> waitForDisconnected(std::chrono::milliseconds timeout);

    //! Connects to server and waits until the connection is established.
    /*!
     * Equivalent to calling [`QAbstractSocket::connecToServer`][qdoc-qabstractsocket-connecToHost]
     * followed by [`QAbstractSocket::waitForConnected`][qdoc-qabstractsocket-waitForConnected].
     *
     * Returns `true` if the connection has been successfully established withint he given timeout,
     * `false` otherwise. If the timeout is -1, the operation will never time out.
     *
     * [qtdoc-qabstractsocket-connectToHost]: https://doc.qt.io/qt-5/qabstractsocket.html#connectToHost
     * [qtdoc-qabstractsocket-waitForConnected]: https://doc.qt.io/qt-5/qabstractsocket.html#waitForConnected
     */
    Task<bool> connectToHost(const QString &hostName, quint16 port,
                             QIODevice::OpenMode openMode = QIODevice::ReadWrite,
                             QAbstractSocket::NetworkLayerProtocol protocol = QAbstractSocket::AnyIPProtocol,
                             std::chrono::milliseconds timeout = std::chrono::seconds{30});

    //! Connects to server and waits until the connection is established.
    /*!
     * Equivalent to calling [`QAbstractSocket::connecToServer`][qdoc-qabstractsocket-connecToHost-1]
     * followed by [`QAbstractSocket::waitForConnected`][qdoc-qabstractsocket-waitForConnected].
     *
     * Returns `true` if the connection has been successfully established within the given timeout,
     * `false` otherwise. If the timeout is -1, the operation will never time out.
     *
     * [qtdoc-qabstractsocket-waitForConnected]: https://doc.qt.io/qt-5/qabstractsocket.html#waitForConnected
     * [qtdoc-qabstractsocket-connectToHost-1]: https://doc.qt.io/qt-5/qabstractsocket.html#connectToHost-1
     */
    Task<bool> connectToHost(const QHostAddress &address, quint16 port,
                             QIODevice::OpenMode openMode = QIODevice::ReadWrite,
                             std::chrono::milliseconds timeout = std::chrono::seconds{30});

private:
    Task<std::optional<bool>> waitForReadyReadImpl(std::chrono::milliseconds timeout) override;
    Task<std::optional<qint64>> waitForBytesWrittenImpl(std::chrono::milliseconds timeout) override;
};

} // namespace QCoro::detail

//! Returns a coroutine-friendly wrapper for QAbstractSocket object.
/*!
 * Returns a wrapper for the QAbstractSocket \c s that provides coroutine-friendly
 * way to co_await the socket to connect and disconnect.
 *
 * @see docs/reference/qabstractsocket.md
 */
inline auto qCoro(QAbstractSocket &s) noexcept {
    return QCoro::detail::QCoroAbstractSocket{&s};
}
//! \copydoc qCoro(QAbstractSocket &s) noexcept
inline auto qCoro(QAbstractSocket *s) noexcept {
    return QCoro::detail::QCoroAbstractSocket{s};
}

