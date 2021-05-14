// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "impl/waitoperationbase.h"
#include "qcoroiodevice.h"

#include <QAbstractSocket>
#include <QPointer>

#include <chrono>

namespace QCoro::detail {

using namespace std::chrono_literals;

//! QAbstractSocket wrapper with co_awaitable-friendly API.
class QCoroAbstractSocket final : private QCoroIODevice {
    //! An Awaitable that suspends the coroutine until the socket is connected
    class WaitForConnectedOperation : public WaitOperationBase<QAbstractSocket> {
    public:
        WaitForConnectedOperation(QAbstractSocket *socket, int timeout_msecs = 30'000)
            : WaitOperationBase(socket, timeout_msecs) {}

        bool await_ready() const noexcept {
            return !mObj || mObj->state() == QAbstractSocket::ConnectedState;
        }

        void await_suspend(QCORO_STD::coroutine_handle<> awaitingCoroutine) noexcept {
            mConn = QObject::connect(mObj, &QAbstractSocket::stateChanged,
                                     [this, awaitingCoroutine](auto newState) mutable {
                                         switch (newState) {
                                         case QAbstractSocket::UnconnectedState:
                                         case QAbstractSocket::HostLookupState:
                                         case QAbstractSocket::ConnectingState:
                                         case QAbstractSocket::BoundState:
                                             // Almost there...
                                             break;
                                         case QAbstractSocket::ClosingState:
                                         case QAbstractSocket::ListeningState:
                                             // We shouldn't be here when waiting for Connected state...
                                             resume(awaitingCoroutine);
                                             break;
                                         case QAbstractSocket::ConnectedState:
                                             resume(awaitingCoroutine);
                                             break;
                                         }
                                     });

            startTimeoutTimer(awaitingCoroutine);
        }
    };

    //! An Awaitable that suspends the coroutine until the socket is disconnected
    class WaitForDisconnectedOperation : public WaitOperationBase<QAbstractSocket> {
    public:
        WaitForDisconnectedOperation(QAbstractSocket *socket, int timeout_msecs)
            : WaitOperationBase(socket, timeout_msecs) {}

        bool await_ready() const noexcept {
            return !mObj || mObj->state() == QAbstractSocket::UnconnectedState;
        }

        void await_suspend(QCORO_STD::coroutine_handle<> awaitingCoroutine) {
            mConn = QObject::connect(
                mObj, &QAbstractSocket::disconnected,
                [this, awaitingCoroutine]() mutable { resume(awaitingCoroutine); });
            startTimeoutTimer(awaitingCoroutine);
        }
    };

    class ReadOperation final : public QCoroIODevice::ReadOperation {
    public:
        using QCoroIODevice::ReadOperation::ReadOperation;

        bool await_ready() const noexcept final {
            return QCoroIODevice::ReadOperation::await_ready() ||
                   static_cast<const QAbstractSocket *>(mDevice.data())->state() ==
                       QAbstractSocket::UnconnectedState;
        }

        void await_suspend(QCORO_STD::coroutine_handle<> awaitingCoroutine) noexcept {
            QCoroIODevice::ReadOperation::await_suspend(awaitingCoroutine);
            mStateConn = QObject::connect(
                static_cast<QAbstractSocket *>(mDevice.data()), &QAbstractSocket::stateChanged,
                [this, awaitingCoroutine]() {
                    if (static_cast<const QAbstractSocket *>(mDevice.data())->state() ==
                        QAbstractSocket::UnconnectedState) {
                        finish(awaitingCoroutine);
                    }
                });
        }

    private:
        void finish(QCORO_STD::coroutine_handle<> awaitingCoroutine) final {
            QObject::disconnect(mStateConn);
            QCoroIODevice::ReadOperation::finish(awaitingCoroutine);
        }

        QMetaObject::Connection mStateConn;
    };

public:
    explicit QCoroAbstractSocket(QAbstractSocket *socket) : QCoroIODevice(socket) {}

    //! Co_awaitable equivalent  to [`QAbstractSocket::waitForConnected()`][qtdoc-qabstractsocket-waitForConnected].
    Awaitable auto waitForConnected(int timeout_msecs = 30'000) {
        return WaitForConnectedOperation{static_cast<QAbstractSocket *>(mDevice.data()),
                                         timeout_msecs};
    }
    //
    //! Co_awaitable equivalent to [`QAbstractSocket::waitForConnected()`][qtdoc-qabstractsocket-waitForConnected].
    /*!
     * Unlike the Qt version, this overload uses `std::chrono::milliseconds` to express the
     * timeout rather than plain `int`.
     */
    Awaitable auto waitForConnected(std::chrono::milliseconds timeout) {
        return waitForConnected(static_cast<int>(timeout.count()));
    }

    //! Co_awaitable equivalent to [`QAbstractSocket::waitForDisconnected()`][qtdoc-qabstractsocket-waitForDisconnected].
    Awaitable auto waitForDisconnected(int timeout_msecs = 30'000) {
        return WaitForDisconnectedOperation{static_cast<QAbstractSocket *>(mDevice.data()),
                                            timeout_msecs};
    }

    //! Co_awaitable equivalent to [`QAbstractSocket::waitForDisconnected()`][qtdoc-qabstractsocket-waitForDisconnected].
    /*!
     * Unlike the Qt version, this overload uses `std::chrono::milliseconds` to express the
     * timeout rather than plain `int`.
     */
    Awaitable auto waitForDisconnected(std::chrono::milliseconds timeout) {
        return waitForDisconnected(static_cast<int>(timeout.count()));
    }

    //! Connects to server and waits until the connection is established.
    /*!
     * Equivalent to calling [`QAbstractSocket::connecToServer`][qdoc-qabstractsocket-connecToHost]
     * followed by [`QAbstractSocket::waitForConnected`][qdoc-qabstractsocket-waitForConnected].
     */
    Awaitable auto
    connectToHost(const QString &hostName, quint16 port,
                  QIODevice::OpenMode openMode = QIODevice::ReadWrite,
                  QAbstractSocket::NetworkLayerProtocol protocol = QAbstractSocket::AnyIPProtocol) {
        static_cast<QAbstractSocket *>(mDevice.data())
            ->connectToHost(hostName, port, openMode, protocol);
        return waitForConnected();
    }

    //! Connects to server and waits until the connection is established.
    /*!
     * Equivalent to calling [`QAbstractSocket::connecToServer`][qdoc-qabstractsocket-connecToHost-1]
     * followed by [`QAbstractSocket::waitForConnected`][qdoc-qabstractsocket-waitForConnected].
     */
    Awaitable auto connectToHost(const QHostAddress &address, quint16 port,
                                 QIODevice::OpenMode openMode = QIODevice::ReadWrite) {
        static_cast<QAbstractSocket *>(mDevice.data())->connectToHost(address, port, openMode);
        return waitForConnected();
    }

    //! \copydoc QCoroIODevice::readAll
    ReadOperation readAll() {
        return ReadOperation(mDevice, [](QIODevice *dev) { return dev->readAll(); });
    }

    //! \copydoc QCoroIODevice::read
    ReadOperation read(qint64 maxSize) {
        return ReadOperation(mDevice, [maxSize](QIODevice *dev) { return dev->read(maxSize); });
    }

    //! \copydoc QCoroIODevice::readLine
    ReadOperation readLine(qint64 maxSize = 0) {
        return ReadOperation(mDevice, [maxSize](QIODevice *dev) { return dev->readLine(maxSize); });
    }
};

} // namespace QCoro::detail

/*!
 * [qtdoc-qabstractsocket-waitForConnected]: https://doc.qt.io/qt-5/qabstractsocket.html#waitForConnected
 * [qtdoc-qabstractsocket-waitForDisconnected]: https://doc.qt.io/qt-5/qabstractsocket.hmtl#waitForDisconnected
 * [qtdoc-qabstractsocket-connectToHost]: https://doc.qt.io/qt-5/qabstractsocket.html#connectToHost
 * [qtdoc-qabstractsocket-connectTo-1]: https://doc.qt.io/qt-5/qabstractsocket.html#connectToHost-1
 */
