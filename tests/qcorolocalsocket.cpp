// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "testhttpserver.h"
#include "testobject.h"
#include "qcoroiodevice_macros.h"

#include "qcoro/network/qcorolocalsocket.h"

#include <QLocalServer>
#include <QLocalSocket>

#include <thread>

class QCoroLocalSocketTest : public QCoro::TestObject<QCoroLocalSocketTest> {
    Q_OBJECT

private:
    QCoro::Task<> testWaitForConnectedTriggers_coro(QCoro::TestContext) {
        QLocalSocket socket;
        QCORO_DELAY(socket.connectToServer(QCoroLocalSocketTest::getSocketName()));

        co_await qCoro(socket).waitForConnected();

        QCORO_COMPARE(socket.state(), QLocalSocket::ConnectedState);
    }

    QCoro::Task<> testWaitForDisconnectedTriggers_coro(QCoro::TestContext) {
        QLocalSocket socket;
        socket.connectToServer(QCoroLocalSocketTest::getSocketName());
        QCORO_COMPARE(socket.state(), QLocalSocket::ConnectedState);

        QCORO_DELAY(socket.disconnectFromServer());

        co_await qCoro(socket).waitForDisconnected();

        QCORO_COMPARE(socket.state(), QLocalSocket::UnconnectedState);
    }

    // On Linux at least, QLocalSocket connects immediately and synchronously
    QCoro::Task<> testDoesntCoAwaitConnectedSocket_coro(QCoro::TestContext context) {
        context.setShouldNotSuspend();

        QLocalSocket socket;
        socket.connectToServer(QCoroLocalSocketTest::getSocketName());

        QCORO_COMPARE(socket.state(), QLocalSocket::ConnectedState);

        co_await qCoro(socket).waitForConnected();
    }

    QCoro::Task<> testDoesntCoAwaitDisconnectedSocket_coro(QCoro::TestContext context) {
        context.setShouldNotSuspend();
        mServer.setExpectTimeout(true);

        QLocalSocket socket;
        QCORO_COMPARE(socket.state(), QLocalSocket::UnconnectedState);

        co_await qCoro(socket).waitForDisconnected();
    }

    QCoro::Task<> testConnectToServerWithArgs_coro(QCoro::TestContext context) {
        context.setShouldNotSuspend();

        QLocalSocket socket;

        co_await qCoro(socket).connectToServer(QCoroLocalSocketTest::getSocketName());

        QCORO_COMPARE(socket.state(), QLocalSocket::ConnectedState);
    }

    QCoro::Task<> testConnectToServer_coro(QCoro::TestContext context) {
        context.setShouldNotSuspend();

        QLocalSocket socket;
        socket.setServerName(QCoroLocalSocketTest::getSocketName());

        co_await qCoro(socket).connectToServer();

        QCORO_COMPARE(socket.state(), QLocalSocket::ConnectedState);
    }

    QCoro::Task<> testWaitForConnectedTimeout_coro(QCoro::TestContext) {
        mServer.setExpectTimeout(true);

        QLocalSocket socket;

        QCORO_TEST_TIMEOUT(co_await qCoro(socket).waitForConnected(10ms));
    }

    QCoro::Task<> testWaitForDisconnectedTimeout_coro(QCoro::TestContext) {
        QLocalSocket socket;
        socket.connectToServer(QCoroLocalSocketTest::getSocketName());
        QCORO_COMPARE(socket.state(), QLocalSocket::ConnectedState);

        QCORO_TEST_TIMEOUT(co_await qCoro(socket).waitForDisconnected(10ms));
    }

    QCoro::Task<> testReadAllTriggers_coro(QCoro::TestContext) {
        QLocalSocket socket;
        socket.connectToServer(QCoroLocalSocketTest::getSocketName());
        QCORO_COMPARE(socket.state(), QLocalSocket::ConnectedState);

        socket.write("GET /stream HTTP/1.1\r\n");

        QCORO_TEST_IODEVICE_READALL(socket);
    }

    QCoro::Task<> testReadTriggers_coro(QCoro::TestContext) {
        QLocalSocket socket;
        socket.connectToServer(QCoroLocalSocketTest::getSocketName());
        QCORO_COMPARE(socket.state(), QLocalSocket::ConnectedState);

        socket.write("GET /stream HTTP/1.1\r\n");

        QCORO_TEST_IODEVICE_READ(socket);
    }

    QCoro::Task<> testReadLineTriggers_coro(QCoro::TestContext) {
        QLocalSocket socket;
        socket.connectToServer(QCoroLocalSocketTest::getSocketName());
        QCORO_COMPARE(socket.state(), QLocalSocket::ConnectedState);

        socket.write("GET /stream HTTP/1.1\r\n");

        QCORO_TEST_IODEVICE_READLINE(socket);
        QCORO_COMPARE(lines.size(), 14);
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
    addTest(ReadAllTriggers)
    addTest(ReadTriggers)
    addTest(ReadLineTriggers)

private:
    static QString getSocketName() {

        return QStringLiteral("%1-%2")
            .arg(QCoreApplication::applicationName())
            .arg(QCoreApplication::applicationPid());
    }

    TestHttpServer<QLocalServer> mServer;
};

QTEST_GUILESS_MAIN(QCoroLocalSocketTest)

#include "qcorolocalsocket.moc"
