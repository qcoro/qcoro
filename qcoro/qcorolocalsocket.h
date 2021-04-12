// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "qcoroiodevice.h"
#include "impl/waitoperationbase.h"

#include <QLocalSocket>
#include <QPointer>

#include <chrono>

namespace QCoro::detail {

using namespace std::chrono_literals;

//! QLocalSocket wrapper with co_awaitable-friendly API.
class QCoroLocalSocket: public QCoroIODevice {
    //! An Awaitable that suspends the coroutine until the socket is connected
    class WaitForConnectedOperation : public WaitOperationBase<QLocalSocket> {
    public:
        WaitForConnectedOperation(QLocalSocket *socket, int timeout_msecs = 30'000)
            : WaitOperationBase(socket, timeout_msecs) {}

        bool await_ready() const noexcept {
            return !mObj || mObj->state() == QLocalSocket::ConnectedState;
        }

        void await_suspend(QCORO_STD::coroutine_handle<> awaitingCoroutine) noexcept {
            mConn = QObject::connect(mObj, &QLocalSocket::stateChanged,
                [this, awaitingCoroutine](auto newState) mutable {
                    switch (newState) {
                        case QLocalSocket::UnconnectedState:
                        case QLocalSocket::ConnectingState:
                            // Almost there...
                            break;
                        case QLocalSocket::ClosingState:
                            // We shouldn't be here when waiting for Connected state...
                            resume(awaitingCoroutine);
                            break;
                        case QLocalSocket::ConnectedState:
                            resume(awaitingCoroutine);
                            break;
                        }
                });

            startTimeoutTimer(awaitingCoroutine);
        }
    };

    //! An Awaitable that suspends the coroutine until the socket is disconnected
    class WaitForDisconnectedOperation : public WaitOperationBase<QLocalSocket> {
    public:
        WaitForDisconnectedOperation(QLocalSocket *socket, int timeout_msecs)
            : WaitOperationBase(socket, timeout_msecs) {}

        bool await_ready() const noexcept {
            return !mObj || mObj->state() == QLocalSocket::UnconnectedState;
        }

        void await_suspend(QCORO_STD::coroutine_handle<> awaitingCoroutine) {
            mConn = QObject::connect(mObj, &QLocalSocket::disconnected,
                [this, awaitingCoroutine]() mutable {
                    resume(awaitingCoroutine);
                });
            startTimeoutTimer(awaitingCoroutine);
        }
    };

public:
    explicit QCoroLocalSocket(QLocalSocket *socket)
        : QCoroIODevice(socket)
    {}

    //! Co_awaitable equivalent  to [`QLocalSocket::waitForConnected()`][qtdoc-qlocalsocket-waitForConnected].
    Awaitable auto waitForConnected(int timeout_msecs = 30'000) {
        return WaitForConnectedOperation{static_cast<QLocalSocket *>(mDevice.data()), timeout_msecs};
    }
    //
    //! Co_awaitable equivalent to [`QLocalSocket::waitForConnected()`][qtdoc-qlocalsocket-waitForConnected].
    /*!
     * Unlike the Qt version, this overload uses `std::chrono::milliseconds` to express the
     * timeout rather than plain `int`.
     */
    Awaitable auto waitForConnected(std::chrono::milliseconds timeout) {
        return waitForConnected(static_cast<int>(timeout.count()));
    }

    //! Co_awaitable equivalent to [`QLocalSocket::waitForDisconnected()`][qtdoc-qlocalsocket-waitForDisconnected].
    Awaitable auto waitForDisconnected(int timeout_msecs = 30'000) {
        return WaitForDisconnectedOperation{static_cast<QLocalSocket *>(mDevice.data()), timeout_msecs};
    }

    //! Co_awaitable equivalent to [`QLocalSocket::waitForDisconnected()`][qtdoc-qlocalsocket-waitForDisconnected].
    /*!
     * Unlike the Qt version, this overload uses `std::chrono::milliseconds` to express the
     * timeout rather than plain `int`.
     */
    Awaitable auto waitForDisconnected(std::chrono::milliseconds timeout) {
        return waitForDisconnected(static_cast<int>(timeout.count()));
    }

    //! Connects to server and waits until the connection is established.
    /*!
     * Equivalent to calling [`QLocalSocket::connecToServer`][qdoc-qlocalsocket-connecToServer]
     * followed by [`QLocalSocket::waitForConnected`][qdoc-qlocalsocket-waitForConnected].
     */
    Awaitable auto connectToServer(QIODevice::OpenMode openMode = QIODevice::ReadWrite) {
        static_cast<QLocalSocket *>(mDevice.data())->connectToServer(openMode);
        return waitForConnected();
    }

    //! Connects to server and waits until the connection is established.
    /*!
     * Equivalent to calling [`QLocalSocket::connecToServer`][qdoc-qlocalsocket-connecToServer]
     * followed by [`QLocalSocket::waitForConnected`][qdoc-qlocalsocket-waitForConnected].
     */
    Awaitable auto connectToServer(const QString &name, QIODevice::OpenMode openMode = QIODevice::ReadWrite) {
        static_cast<QLocalSocket *>(mDevice.data())->connectToServer(name, openMode);
        return waitForConnected();
    }
};

} // namespace QCoro::detail

/*!
 * [qtdoc-qlocalsocket-waitForConnected]: https://doc.qt.io/qt-5/qlocalsocket.html#waitForConnected
 * [qtdoc-qlocalsocket-waitForDisconnected]: https://doc.qt.io/qt-5/qlocalsocket.hmtl#waitForDisconnected
 * [qtdoc-qlocalsocket-connectToServer]: https://doc.qt.io/qt-5/qlocalsocket.html#connectToServer
 * [qtdoc-qlocalsocket-connectToServer-1]: https://doc.qt.io/qt-5/qlocalsocket.html#connectToServer-1
 */

