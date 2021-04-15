// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "testobject.h"
#include "testhttpserver.h"
#include "qcoro/coro.h"

#include <QTcpSocket>
#include <QTcpServer>

#include <thread>

class QCoroAbstractSocketTest: public QCoro::TestObject<QCoroAbstractSocketTest>
{
    Q_OBJECT

private:
    QCoro::Task<> testWaitForConnectedTriggers_coro(QCoro::TestContext context) {
        QTcpSocket socket;
        QTimer::singleShot(10ms, [this, &socket]() mutable {
            socket.connectToHost(QHostAddress::LocalHost, mServer.port());
        });

        co_await QCoro::coro(socket).waitForConnected();

        QCORO_COMPARE(socket.state(), QAbstractSocket::ConnectedState);
    }

    QCoro::Task<> testWaitForDisconnectedTriggers_coro(QCoro::TestContext context) {
        QTcpSocket socket;
        co_await QCoro::coro(socket).connectToHost(QHostAddress::LocalHost, mServer.port());
        QCORO_COMPARE(socket.state(), QAbstractSocket::ConnectedState);

        QTimer::singleShot(10ms, [&socket]() mutable {
            socket.disconnectFromHost();
        });

        co_await QCoro::coro(socket).waitForDisconnected();

        QCORO_COMPARE(socket.state(), QAbstractSocket::UnconnectedState);
    }

    QCoro::Task<> testDoesntCoAwaitConnectedSocket_coro(QCoro::TestContext context) {
        QTcpSocket socket;
        co_await QCoro::coro(socket).connectToHost(QHostAddress::LocalHost, mServer.port());

        QCORO_COMPARE(socket.state(), QAbstractSocket::ConnectedState);

        context.setShouldNotSuspend();
        co_await QCoro::coro(socket).waitForConnected();
    }

    QCoro::Task<> testDoesntCoAwaitDisconnectedSocket_coro(QCoro::TestContext context) {
        context.setShouldNotSuspend();

        QTcpSocket socket;
        QCORO_COMPARE(socket.state(), QAbstractSocket::UnconnectedState);

        co_await QCoro::coro(socket).waitForDisconnected();
    }

    QCoro::Task<> testConnectToServerWithArgs_coro(QCoro::TestContext context) {
        QTcpSocket socket;

        co_await QCoro::coro(socket).connectToHost(QHostAddress::LocalHost, mServer.port());

        QCORO_COMPARE(socket.state(), QAbstractSocket::ConnectedState);
    }

    QCoro::Task<> testWaitForConnectedTimeout_coro(QCoro::TestContext context) {
        QTcpSocket socket;

        const bool ok = co_await QCoro::coro(socket).waitForConnected(10ms);
        QCORO_VERIFY(!ok);
    }

    QCoro::Task<> testWaitForDisconnectedTimeout_coro(QCoro::TestContext context) {
        QTcpSocket socket;
        co_await QCoro::coro(socket).connectToHost(QHostAddress::LocalHost, mServer.port());
        QCORO_COMPARE(socket.state(), QAbstractSocket::ConnectedState);

        const bool ok = co_await QCoro::coro(socket).waitForDisconnected(10ms);
        QCORO_VERIFY(!ok);
    }

    QCoro::Task<> testReadAllTriggers_coro(QCoro::TestContext context) {
        QTcpSocket socket;
        co_await QCoro::coro(socket).connectToHost(QHostAddress::LocalHost, mServer.port());
        QCORO_COMPARE(socket.state(), QAbstractSocket::ConnectedState);

        socket.write("GET /stream HTTP/1.1\r\n");

        QByteArray data;
        while (socket.state() == QAbstractSocket::ConnectedState) {
            data += co_await QCoro::coro(socket).readAll();
        }
        QCORO_VERIFY(!data.isEmpty());
        data += socket.readAll(); // read what's left in the buffer

        QCORO_VERIFY(!data.isEmpty());
    }

    QCoro::Task<> testReadTriggers_coro(QCoro::TestContext context) {
        QTcpSocket socket;
        co_await QCoro::coro(socket).connectToHost(QHostAddress::LocalHost, mServer.port());
        QCORO_COMPARE(socket.state(), QAbstractSocket::ConnectedState);

        socket.write("GET /stream HTTP/1.1\r\n");

        QByteArray data;
        while (socket.state() == QAbstractSocket::ConnectedState) {
            data += co_await QCoro::coro(socket).read(1);
        }
        QCORO_VERIFY(!data.isEmpty());
        data += socket.readAll(); // read what's left in the buffer

        QCORO_VERIFY(!data.isEmpty());
    }

    QCoro::Task<> testReadLineTriggers_coro(QCoro::TestContext context) {
        QTcpSocket socket;
        co_await QCoro::coro(socket).connectToHost(QHostAddress::LocalHost, mServer.port());
        QCORO_COMPARE(socket.state(), QAbstractSocket::ConnectedState);

        socket.write("GET /stream HTTP/1.1\r\n");

        QByteArrayList lines;
        while (socket.state() == QAbstractSocket::ConnectedState) {
            const auto line = co_await QCoro::coro(socket).readLine();
            if (!line.isNull()) {
                lines.push_back(line);
            }
        }

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
