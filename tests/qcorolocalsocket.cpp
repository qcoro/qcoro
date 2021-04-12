// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "testobject.h"
#include "testhttpserver.h"
#include "qcoro/coro.h"

#include <QLocalServer>

#include <thread>

class QCoroLocalSocketTest: public QCoro::TestObject<QCoroLocalSocketTest>
{
    Q_OBJECT

private:
    QCoro::Task<> testWaitForConnectedTriggers_coro(QCoro::TestContext context) {
        // Must be deleted later, otherwise when the coroutine is resumed from the QTimer slot,
        // the coroutine ends and the QLocalSocket is effectively destroyed from within its own
        // stateChanged signal handler.
        QScopedPointer<QLocalSocket, QScopedPointerDeleteLater> socket(new QLocalSocket);
        // Invoke the connect delayed - connectToServer() on Linux is synchronous,
        // so the co_await wouldn't suspend the coroutine.
        QTimer::singleShot(10ms, [&socket]() mutable {
            socket->connectToServer(QCoroLocalSocketTest::getSocketName());
        });

        co_await QCoro::coro(socket.get()).waitForConnected();

        QCORO_COMPARE(socket->state(), QLocalSocket::ConnectedState);
    }

    QCoro::Task<> testWaitForDisconnectedTriggers_coro(QCoro::TestContext context) {
        // Must be deleted from event loop, see the comment in testWaitForConnectedTriggers_coro()
        QScopedPointer<QLocalSocket, QScopedPointerDeleteLater> socket(new QLocalSocket);
        socket->connectToServer(QCoroLocalSocketTest::getSocketName());
        QCORO_COMPARE(socket->state(), QLocalSocket::ConnectedState);

        QTimer::singleShot(10ms, [&socket]() mutable {
            socket->disconnectFromServer();
        });

        co_await QCoro::coro(socket.get()).waitForDisconnected();

        QCORO_COMPARE(socket->state(), QLocalSocket::UnconnectedState);
    }

    // On Linux at least, QLocalSocket connects immediately and synchronously
    QCoro::Task<> testDoesntCoAwaitConnectedSocket_coro(QCoro::TestContext context) {
        context.setShouldNotSuspend();

        QLocalSocket socket;
        socket.connectToServer(QCoroLocalSocketTest::getSocketName());

        QCORO_COMPARE(socket.state(), QLocalSocket::ConnectedState);

        co_await QCoro::coro(socket).waitForConnected();
    }

    QCoro::Task<> testDoesntCoAwaitDisconnectedSocket_coro(QCoro::TestContext context) {
        context.setShouldNotSuspend();

        QLocalSocket socket;
        QCORO_COMPARE(socket.state(), QLocalSocket::UnconnectedState);

        co_await QCoro::coro(socket).waitForDisconnected();
    }

    QCoro::Task<> testConnectToServerWithArgs_coro(QCoro::TestContext context) {
        context.setShouldNotSuspend();

        QLocalSocket socket;

        co_await QCoro::coro(socket).connectToServer(QCoroLocalSocketTest::getSocketName());

        QCORO_COMPARE(socket.state(), QLocalSocket::ConnectedState);
    }

    QCoro::Task<> testConnectToServer_coro(QCoro::TestContext context) {
        context.setShouldNotSuspend();

        QLocalSocket socket;
        socket.setServerName(QCoroLocalSocketTest::getSocketName());

        co_await QCoro::coro(socket).connectToServer();

        QCORO_COMPARE(socket.state(), QLocalSocket::ConnectedState);
    }

    QCoro::Task<> testWaitForConnectedTimeout_coro(QCoro::TestContext context) {
        QLocalSocket socket;

        const bool ok = co_await QCoro::coro(socket).waitForConnected(10ms);
        QCORO_VERIFY(!ok);
    }

    QCoro::Task<> testWaitForDisconnectedTimeout_coro(QCoro::TestContext context) {
        QLocalSocket socket;
        socket.connectToServer(QCoroLocalSocketTest::getSocketName());
        QCORO_COMPARE(socket.state(), QLocalSocket::ConnectedState);

        const bool ok = co_await QCoro::coro(socket).waitForDisconnected(10ms);
        QCORO_VERIFY(!ok);
    }

private Q_SLOTS:
    void init() {
        mServer.start(QCoroLocalSocketTest::getSocketName());
    }

    void cleanup() {
        mServer.stop();
    }


    addTest(WaitForConnectedTriggers)
    addTest(WaitForConnectedTimeout)
    addTest(WaitForDisconnectedTriggers)
    addTest(WaitForDisconnectedTimeout)
    addTest(DoesntCoAwaitConnectedSocket)
    addTest(DoesntCoAwaitDisconnectedSocket)
    addTest(ConnectToServerWithArgs)
    addTest(ConnectToServer)

private:
    static QString getSocketName() {

        return QStringLiteral("%1-%2").arg(QCoreApplication::applicationName())
                                      .arg(QCoreApplication::applicationPid());
    }


    TestHttpServer<QLocalServer> mServer;
};

QTEST_GUILESS_MAIN(QCoroLocalSocketTest)

#include "qcorolocalsocket.moc"
