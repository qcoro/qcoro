// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "waitoperationbase_p.h"
#include "qcoroiodevice.h"

#include <QPointer>
#include <QAbstractSocket>

#include <chrono>

class QAbstractSocket;

namespace QCoro::detail {

using namespace std::chrono_literals;

//! QAbstractSocket wrapper with co_awaitable-friendly API.
class QCoroAbstractSocket final : private QCoroIODevice {
    class ReadOperation final : public QCoroIODevice::ReadOperation {
    public:
        using QCoroIODevice::ReadOperation::ReadOperation;
        bool await_ready() const noexcept final;
        void await_suspend(std::coroutine_handle<> awaitingCoroutine) noexcept final;

    private:
        void finish(std::coroutine_handle<> awaitingCoroutine);

        QMetaObject::Connection mStateConn;
    };

    class WaitForConnectedOperation final : public WaitOperationBase<QAbstractSocket> {
    public:
        WaitForConnectedOperation(QAbstractSocket *socket, int timeout_msecs = 30'000);
        bool await_ready() const noexcept;
        void await_suspend(std::coroutine_handle<> awaitingCoroutine) noexcept;
    };

    class WaitForDisconnectedOperation final : public WaitOperationBase<QAbstractSocket> {
    public:
        WaitForDisconnectedOperation(QAbstractSocket *socket, int timeout_msecs);
        bool await_ready() const noexcept;
        void await_suspend(std::coroutine_handle<> awaitingCoroutine) noexcept;
    };

public:
    explicit QCoroAbstractSocket(QAbstractSocket *socket);

    //! Co_awaitable equivalent  to [`QAbstractSocket::waitForConnected()`][qtdoc-qabstractsocket-waitForConnected].
    WaitForConnectedOperation waitForConnected(int timeout_msecs = 30'000);

    //! Co_awaitable equivalent to [`QAbstractSocket::waitForConnected()`][qtdoc-qabstractsocket-waitForConnected].
    /*!
     * Unlike the Qt version, this overload uses `std::chrono::milliseconds` to express the
     * timeout rather than plain `int`.
     */
    WaitForConnectedOperation waitForConnected(std::chrono::milliseconds timeout);

    //! Co_awaitable equivalent to [`QAbstractSocket::waitForDisconnected()`][qtdoc-qabstractsocket-waitForDisconnected].
    WaitForDisconnectedOperation waitForDisconnected(int timeout_msecs = 30'000);

    //! Co_awaitable equivalent to [`QAbstractSocket::waitForDisconnected()`][qtdoc-qabstractsocket-waitForDisconnected].
    /*!
     * Unlike the Qt version, this overload uses `std::chrono::milliseconds` to express the
     * timeout rather than plain `int`.
     */
    WaitForDisconnectedOperation waitForDisconnected(std::chrono::milliseconds timeout);

    //! Connects to server and waits until the connection is established.
    /*!
     * Equivalent to calling [`QAbstractSocket::connecToServer`][qdoc-qabstractsocket-connecToHost]
     * followed by [`QAbstractSocket::waitForConnected`][qdoc-qabstractsocket-waitForConnected].
     */
    WaitForConnectedOperation connectToHost(const QString &hostName, quint16 port,
        QIODevice::OpenMode openMode = QIODevice::ReadWrite,
        QAbstractSocket::NetworkLayerProtocol protocol = QAbstractSocket::AnyIPProtocol);

    //! Connects to server and waits until the connection is established.
    /*!
     * Equivalent to calling [`QAbstractSocket::connecToServer`][qdoc-qabstractsocket-connecToHost-1]
     * followed by [`QAbstractSocket::waitForConnected`][qdoc-qabstractsocket-waitForConnected].
     */
    WaitForConnectedOperation connectToHost(const QHostAddress &address, quint16 port,
        QIODevice::OpenMode openMode = QIODevice::ReadWrite);

    //! \copydoc QCoroIODevice::readAll
    ReadOperation readAll();

    //! \copydoc QCoroIODevice::read
    ReadOperation read(qint64 maxSize);

    //! \copydoc QCoroIODevice::readLine
    ReadOperation readLine(qint64 maxSize = 0);
};

} // namespace QCoro::detail

/*!
 * [qtdoc-qabstractsocket-waitForConnected]: https://doc.qt.io/qt-5/qabstractsocket.html#waitForConnected
 * [qtdoc-qabstractsocket-waitForDisconnected]: https://doc.qt.io/qt-5/qabstractsocket.hmtl#waitForDisconnected
 * [qtdoc-qabstractsocket-connectToHost]: https://doc.qt.io/qt-5/qabstractsocket.html#connectToHost
 * [qtdoc-qabstractsocket-connectTo-1]: https://doc.qt.io/qt-5/qabstractsocket.html#connectToHost-1
 */

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

