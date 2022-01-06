// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "waitoperationbase_p.h"

#include <QPointer>

#include <chrono>

class QTcpServer;
class QTcpSocket;

namespace QCoro::detail {

using namespace std::chrono_literals;

//! QTcpServer wrapper with co_awaitable-friendly API.
class QCoroTcpServer {
    //! An Awaitable that suspends the coroutine until new connection is available
    class WaitForNewConnectionOperation final : public WaitOperationBase<QTcpServer> {
    public:
        WaitForNewConnectionOperation(QTcpServer *server, int timeout_msecs = 30'000);
        bool await_ready() const noexcept;
        void await_suspend(std::coroutine_handle<> awaitingCoroutine) noexcept;
        QTcpSocket *await_resume();
    };

public:
    ///! Constructor.
    explicit QCoroTcpServer(QTcpServer *server);

    //! Co_awaitable equivalent to [`QTcpServer::waitForNewConnection()`][qtdoc-qtcpserver-waitForNewConnection].
    WaitForNewConnectionOperation waitForNewConnection(int timeout_msecs = 30'000);

    //! Co_awaitable equivalent to [`QTcpServer::waitForNewConnection()`][qtdoc-qtcpserver-waitForNewConnection].
    /*!
     * Unlike the Qt version, this overload uses `std::chrono::milliseconds` to express the
     * timeout rather than plain `int`.
     */
    WaitForNewConnectionOperation waitForNewConnection(std::chrono::milliseconds timeout);

private:
    QPointer<QTcpServer> mServer;
};

} // namespace QCoro::detail

/*!
 * [qtdoc-qtcpserver-waitForNewConnection]: https://doc.qt.io/qt-5/qtcpserver.html#waitForNewConnection
 */

//! Returns a coroutine-friendly wrapper for QTcpServer object.
/*!
 * Returns a wrapper for QTcpServer \c s that provides coroutine-friendly way
 * of co_awaiting new connections.
 *
 * @see docs/reference/qtcpserver.md
 */
inline auto qCoro(QTcpServer &s) noexcept {
    return QCoro::detail::QCoroTcpServer{&s};
}
//! \copydoc qCoro(QTcpServer &s) noexcept
inline auto qCoro(QTcpServer *s) noexcept {
    return QCoro::detail::QCoroTcpServer{s};
}

