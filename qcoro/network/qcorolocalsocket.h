// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "waitoperationbase_p.h"
#include "qcoroiodevice.h"

#include <QLocalSocket>

#include <chrono>

namespace QCoro::detail {

using namespace std::chrono_literals;

//! QLocalSocket wrapper with co_awaitable-friendly API.
class QCoroLocalSocket : private QCoroIODevice {
    //! An Awaitable that suspends the coroutine until the socket is connected
    class WaitForConnectedOperation final : public WaitOperationBase<QLocalSocket> {
    public:
        explicit WaitForConnectedOperation(QLocalSocket *socket, int timeout_msecs = 30'000);
        bool await_ready() const noexcept;
        void await_suspend(std::coroutine_handle<> awaitingCoroutine) noexcept;
    };

    //! An Awaitable that suspends the coroutine until the socket is disconnected
    class WaitForDisconnectedOperation final : public WaitOperationBase<QLocalSocket> {
    public:
        WaitForDisconnectedOperation(QLocalSocket *socket, int timeout_msecs);
        bool await_ready() const noexcept;
        void await_suspend(std::coroutine_handle<> awaitingCoroutine);
    };

    class ReadOperation final : public QCoroIODevice::ReadOperation {
    public:
        using QCoroIODevice::ReadOperation::ReadOperation;

        bool await_ready() const noexcept final;
        void await_suspend(std::coroutine_handle<> awaitingCoroutine) noexcept final;

    private:
        void finish(std::coroutine_handle<> awaitingCoroutine) final;

        QMetaObject::Connection mStateConn;
    };

public:
    explicit QCoroLocalSocket(QLocalSocket *socket);

    //! Co_awaitable equivalent  to [`QLocalSocket::waitForConnected()`][qtdoc-qlocalsocket-waitForConnected].
    WaitForConnectedOperation waitForConnected(int timeout_msecs = 30'000);

    //! Co_awaitable equivalent to [`QLocalSocket::waitForConnected()`][qtdoc-qlocalsocket-waitForConnected].
    /*!
     * Unlike the Qt version, this overload uses `std::chrono::milliseconds` to express the
     * timeout rather than plain `int`.
     */
    WaitForConnectedOperation waitForConnected(std::chrono::milliseconds timeout);

    //! Co_awaitable equivalent to [`QLocalSocket::waitForDisconnected()`][qtdoc-qlocalsocket-waitForDisconnected].
    WaitForDisconnectedOperation waitForDisconnected(int timeout_msecs = 30'000);

    //! Co_awaitable equivalent to [`QLocalSocket::waitForDisconnected()`][qtdoc-qlocalsocket-waitForDisconnected].
    /*!
     * Unlike the Qt version, this overload uses `std::chrono::milliseconds` to express the
     * timeout rather than plain `int`.
     */
    WaitForDisconnectedOperation waitForDisconnected(std::chrono::milliseconds timeout);

    //! Connects to server and waits until the connection is established.
    /*!
     * Equivalent to calling [`QLocalSocket::connecToServer`][qdoc-qlocalsocket-connecToServer]
     * followed by [`QLocalSocket::waitForConnected`][qdoc-qlocalsocket-waitForConnected].
     */
    WaitForConnectedOperation connectToServer(QIODevice::OpenMode openMode = QIODevice::ReadWrite);

    //! Connects to server and waits until the connection is established.
    /*!
     * Equivalent to calling [`QLocalSocket::connecToServer`][qdoc-qlocalsocket-connecToServer]
     * followed by [`QLocalSocket::waitForConnected`][qdoc-qlocalsocket-waitForConnected].
     */
    WaitForConnectedOperation connectToServer(const QString &name, QIODevice::OpenMode openMode = QIODevice::ReadWrite);

    //! \copydoc QIODevice::readAll
    ReadOperation readAll();

    //! \copydoc QIODevice::read
    ReadOperation read(qint64 maxSize);

    //! \copydoc QIODevice::readLine
    ReadOperation readLine(qint64 maxSize = 0);
};

} // namespace QCoro::detail

/*!
 * [qtdoc-qlocalsocket-waitForConnected]: https://doc.qt.io/qt-5/qlocalsocket.html#waitForConnected
 * [qtdoc-qlocalsocket-waitForDisconnected]: https://doc.qt.io/qt-5/qlocalsocket.hmtl#waitForDisconnected
 * [qtdoc-qlocalsocket-connectToServer]: https://doc.qt.io/qt-5/qlocalsocket.html#connectToServer
 * [qtdoc-qlocalsocket-connectToServer-1]: https://doc.qt.io/qt-5/qlocalsocket.html#connectToServer-1
 */

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

