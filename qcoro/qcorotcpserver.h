// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "impl/waitoperationbase.h"

#include <QTcpServer>
#include <QPointer>

#include <chrono>

namespace QCoro::detail {

using namespace std::chrono_literals;

//! QTcpServer wrapper with co_awaitable-friendly API.
class QCoroTcpServer {
    //! An Awaitable that suspends the coroutine until new connection is available
    class WaitForNewConnectionOperation final : public WaitOperationBase<QTcpServer> {
    public:
        WaitForNewConnectionOperation(QTcpServer *server, int timeout_msecs = 30'000)
            : WaitOperationBase(server, timeout_msecs) {}

        bool await_ready() const noexcept {
            return !mObj || mObj->hasPendingConnections();
        }

        void await_suspend(QCORO_STD::coroutine_handle<> awaitingCoroutine) noexcept {
            mConn = QObject::connect(mObj, &QTcpServer::newConnection,
                                     [this, awaitingCoroutine]() mutable {
                                        resume(awaitingCoroutine);
                                     });
            startTimeoutTimer(awaitingCoroutine);
        }

        QTcpSocket *await_resume() {
            return mTimedOut ? nullptr : mObj->nextPendingConnection();
        }
    };

public:
    explicit QCoroTcpServer(QTcpServer *server)
        : mServer(server) {}

    //! Co_awaitable equivalent to [`QTcpServer::waitForNewConnection()`][qtdoc-qtcpserver-waitForNewConnection].
    Awaitable auto waitForNewConnection(int timeout_msecs = 30'000) {
        return WaitForNewConnectionOperation{mServer.data(), timeout_msecs};
    }

    //! Co_awaitable equivalent to [`QTcpServer::waitForNewConnection()`][qtdoc-qtcpserver-waitForNewConnection].
    /*!
     * Unlike the Qt version, this overload uses `std::chrono::milliseconds` to express the
     * timeout rather than plain `int`.
     */
    Awaitable auto waitForNewConnection(std::chrono::milliseconds timeout) {
        return WaitForNewConnectionOperation{mServer.data(), static_cast<int>(timeout.count())};
    }

private:
    QPointer<QTcpServer> mServer;
};

} // namespace QCoro::detail

/*!
 * [qtdoc-qtcpserver-waitForNewConnection]: https://doc.qt.io/qt-5/qtcpserver.html#waitForNewConnection
 */
