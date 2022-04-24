// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "testhttpserver.h"
#include "testobject.h"
#include "qcoroiodevice_macros.h"

#include "qcoro/network/qcoroabstractsocket.h"

#include <QTcpServer>
#include <QTcpSocket>

#include <thread>

class QCoroAbstractSocketTest : public QCoro::TestObject<QCoroAbstractSocketTest> {
    Q_OBJECT

private:
    QCoro::Task<> testWaitForConnectedTriggers_coro(QCoro::TestContext) {
        QTcpSocket socket;
        QCORO_DELAY(socket.connectToHost(QHostAddress::LocalHost, mServer.port()));

        co_await qCoro(socket).waitForConnected();

        QCORO_COMPARE(socket.state(), QAbstractSocket::ConnectedState);
    }

    QCoro::Task<> testWaitForDisconnectedTriggers_coro(QCoro::TestContext) {
        QTcpSocket socket;
        co_await qCoro(socket).connectToHost(QHostAddress::LocalHost, mServer.port());
        QCORO_COMPARE(socket.state(), QAbstractSocket::ConnectedState);

        QCORO_DELAY(socket.disconnectFromHost());

        co_await qCoro(socket).waitForDisconnected();

        QCORO_COMPARE(socket.state(), QAbstractSocket::UnconnectedState);
    }

    QCoro::Task<> testDoesntCoAwaitConnectedSocket_coro(QCoro::TestContext context) {
        QTcpSocket socket;
        co_await qCoro(socket).connectToHost(QHostAddress::LocalHost, mServer.port());

        QCORO_COMPARE(socket.state(), QAbstractSocket::ConnectedState);

        context.setShouldNotSuspend();
        co_await qCoro(socket).waitForConnected();
    }

    QCoro::Task<> testDoesntCoAwaitDisconnectedSocket_coro(QCoro::TestContext context) {
        context.setShouldNotSuspend();
        mServer.setExpectTimeout(true); // no-one actually connects, so the server times out.

        QTcpSocket socket;
        QCORO_COMPARE(socket.state(), QAbstractSocket::UnconnectedState);

        co_await qCoro(socket).waitForDisconnected();
    }

    QCoro::Task<> testConnectToServerWithArgs_coro(QCoro::TestContext) {
        QTcpSocket socket;

        co_await qCoro(socket).connectToHost(QHostAddress::LocalHost, mServer.port());

        QCORO_COMPARE(socket.state(), QAbstractSocket::ConnectedState);
    }

    QCoro::Task<> testWaitForConnectedTimeout_coro(QCoro::TestContext) {
        mServer.setExpectTimeout(true);
        QTcpSocket socket;

        QCORO_TEST_TIMEOUT(co_await qCoro(socket).waitForConnected(10ms));
    }

    QCoro::Task<> testWaitForDisconnectedTimeout_coro(QCoro::TestContext) {
        mServer.setExpectTimeout(true);

        QTcpSocket socket;
        co_await qCoro(socket).connectToHost(QHostAddress::LocalHost, mServer.port());
        QCORO_COMPARE(socket.state(), QAbstractSocket::ConnectedState);

        QCORO_TEST_TIMEOUT(co_await qCoro(socket).waitForDisconnected(10ms));
    }

    QCoro::Task<> testReadAllTriggers_coro(QCoro::TestContext) {
        QTcpSocket socket;
        co_await qCoro(socket).connectToHost(QHostAddress::LocalHost, mServer.port());
        QCORO_COMPARE(socket.state(), QAbstractSocket::ConnectedState);

        socket.write("GET /stream HTTP/1.1\r\n");

        QCORO_TEST_IODEVICE_READALL(socket);
    }

    QCoro::Task<> testReadTriggers_coro(QCoro::TestContext) {
        QTcpSocket socket;
        co_await qCoro(socket).connectToHost(QHostAddress::LocalHost, mServer.port());
        QCORO_COMPARE(socket.state(), QAbstractSocket::ConnectedState);

        socket.write("GET /stream HTTP/1.1\r\n");

        QCORO_TEST_IODEVICE_READ(socket);
    }

    QCoro::Task<> testReadLineTriggers_coro(QCoro::TestContext) {
        QTcpSocket socket;
        co_await qCoro(socket).connectToHost(QHostAddress::LocalHost, mServer.port());
        QCORO_COMPARE(socket.state(), QAbstractSocket::ConnectedState);

        socket.write("GET /stream HTTP/1.1\r\n");

        QCORO_TEST_IODEVICE_READLINE(socket);
        QCORO_COMPARE(lines.size(), 14);
    }

private Q_SLOTS:
    void init() {
        mServer.start(QHostAddress::LocalHost);
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
    addTest(ReadAllTriggers)
    addTest(ReadTriggers)
    addTest(ReadLineTriggers)

private:
    TestHttpServer<QTcpServer> mServer;
};

QTEST_GUILESS_MAIN(QCoroAbstractSocketTest)

#include "qcoroabstractsocket.moc"
