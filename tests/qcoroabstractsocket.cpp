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

        // Make sure the server gets the connection as well
        QCORO_VERIFY(mServer.waitForConnection());
    }

    void testThenWaitForConnectedTriggers_coro(TestLoop &el) {
        QTcpSocket socket;
        QCORO_DELAY(socket.connectToHost(QHostAddress::LocalHost, mServer.port()));
        bool called = false;
        qCoro(socket).waitForConnected().then([&](bool connected) {
            called = true;
            el.quit();
            QVERIFY(connected);
        });
        el.exec();
        QVERIFY(called);
        QVERIFY(mServer.waitForConnection());
    }

    QCoro::Task<> testWaitForDisconnectedTriggers_coro(QCoro::TestContext) {
        QTcpSocket socket;
        co_await qCoro(socket).connectToHost(QHostAddress::LocalHost, mServer.port());
        QCORO_COMPARE(socket.state(), QAbstractSocket::ConnectedState);

        QCORO_DELAY(socket.disconnectFromHost());

        co_await qCoro(socket).waitForDisconnected();

        QCORO_COMPARE(socket.state(), QAbstractSocket::UnconnectedState);

        QCORO_VERIFY(mServer.waitForConnection());
    }

    void testThenWaitForDisconnectedTriggers_coro(TestLoop &el) {
        QTcpSocket socket;
        bool called = false;
        qCoro(socket).connectToHost(QHostAddress::LocalHost, mServer.port()).then([&](bool connected) {
            if (!connected) {
                el.quit();
            }
            QVERIFY(connected);
            QCOMPARE(socket.state(), QAbstractSocket::ConnectedState);

            QCORO_DELAY(socket.disconnectFromHost());

            qCoro(socket).waitForDisconnected().then([&](bool connected) {
                called = true;
                el.quit();
                QVERIFY(connected);
            });
        });
        el.exec();

        QVERIFY(called);

        QVERIFY(mServer.waitForConnection());
    }

    QCoro::Task<> testDoesntCoAwaitConnectedSocket_coro(QCoro::TestContext context) {
        QTcpSocket socket;
        co_await qCoro(socket).connectToHost(QHostAddress::LocalHost, mServer.port());

        QCORO_COMPARE(socket.state(), QAbstractSocket::ConnectedState);

        context.setShouldNotSuspend();
        co_await qCoro(socket).waitForConnected();

        socket.write("GET / HTTP/1.1\r\n");

        QCORO_VERIFY(mServer.waitForConnection());
    }

    void testThenDoesntCoAwaitConnectedSocket_coro(TestLoop &el) {
        QTcpSocket socket;
        bool called = false;
        qCoro(socket).connectToHost(QHostAddress::LocalHost, mServer.port()).then([&](bool connected) {
            if (!connected) {
                el.quit();
            }
            QVERIFY(connected);
            QCOMPARE(socket.state(), QAbstractSocket::ConnectedState);

            qCoro(socket).waitForConnected().then([&](bool connected) {
                called = true;
                el.quit();
                QVERIFY(connected);
            });
        });
        el.exec();

        QVERIFY(called);

        QVERIFY(mServer.waitForConnection());
    }

    QCoro::Task<> testDoesntCoAwaitDisconnectedSocket_coro(QCoro::TestContext context) {
        context.setShouldNotSuspend();
        mServer.setExpectTimeout(true); // no-one actually connects, so the server times out.

        QTcpSocket socket;
        QCORO_COMPARE(socket.state(), QAbstractSocket::UnconnectedState);

        co_await qCoro(socket).waitForDisconnected();
    }

    void testThenDoesntCoAwaitDisconnectedSocket_coro(TestLoop &el) {
        mServer.setExpectTimeout(true); // no-one actually connects, so the server time out.

        QTcpSocket socket;
        QCOMPARE(socket.state(), QAbstractSocket::UnconnectedState);

        bool called = false;
        qCoro(socket).waitForDisconnected().then([&](bool disconnected) {
            called = true;
            el.quit();
            QVERIFY(!disconnected);
        });
        el.exec();

        QVERIFY(called);
    }

    QCoro::Task<> testConnectToServerWithArgs_coro(QCoro::TestContext) {
        QTcpSocket socket;

        co_await qCoro(socket).connectToHost(QHostAddress::LocalHost, mServer.port());

        QCORO_COMPARE(socket.state(), QAbstractSocket::ConnectedState);
        QCORO_VERIFY(mServer.waitForConnection());
    }

    void testThenConnectToServerWithArgs_coro(TestLoop &el) {
        QTcpSocket socket;
        bool called = false;
        qCoro(socket).connectToHost(QHostAddress::LocalHost, mServer.port()).then([&](bool connected) {
            called = true;
            el.quit();
            QVERIFY(connected);
            QCOMPARE(socket.state(), QAbstractSocket::ConnectedState);
        });
        el.exec();
        QVERIFY(called);
        QVERIFY(mServer.waitForConnection());
    }

    QCoro::Task<> testWaitForConnectedTimeout_coro(QCoro::TestContext) {
        mServer.setExpectTimeout(true);
        QTcpSocket socket;

        QCORO_TEST_TIMEOUT(co_await qCoro(socket).waitForConnected(10ms));
    }

    void testThenWaitForConnectedTimeout_coro(TestLoop &el) {
        mServer.setExpectTimeout(true);

        QTcpSocket socket;
        const auto start = std::chrono::steady_clock::now();
        bool called = false;
        qCoro(socket).waitForConnected(10ms).then([&](bool connected) {
            called = true;
            el.quit();
            QVERIFY(!connected);
        });
        el.exec();
        const auto end = std::chrono::steady_clock::now();
        QVERIFY(end - start < 500ms);

        QVERIFY(called);
    }

    QCoro::Task<> testWaitForDisconnectedTimeout_coro(QCoro::TestContext) {
        mServer.setExpectTimeout(true);

        QTcpSocket socket;
        co_await qCoro(socket).connectToHost(QHostAddress::LocalHost, mServer.port());
        QCORO_COMPARE(socket.state(), QAbstractSocket::ConnectedState);

        QCORO_TEST_TIMEOUT(co_await qCoro(socket).waitForDisconnected(10ms));

        QCORO_VERIFY(mServer.waitForConnection());
    }

    void testThenWaitForDisconnectedTimeout_coro(TestLoop &el) {
        mServer.setExpectTimeout(true);

        QTcpSocket socket;
        qCoro(socket).connectToHost(QHostAddress::LocalHost, mServer.port()).then([&](bool connected) {
            if (!connected) {
                el.quit();
            }
            QVERIFY(connected);
            QCOMPARE(socket.state(), QAbstractSocket::ConnectedState);

            const auto start = std::chrono::steady_clock::now();
            qCoro(socket).waitForDisconnected(10ms).then([&el, start](bool disconnected) {
                el.quit();
                QVERIFY(!disconnected);
                const auto end = std::chrono::steady_clock::now();
                QVERIFY(end - start < 500ms);
            });
        });
        el.exec();
        QVERIFY(mServer.waitForConnection());
    }

    QCoro::Task<> testReadAllTriggers_coro(QCoro::TestContext) {
        QTcpSocket socket;
        co_await qCoro(socket).connectToHost(QHostAddress::LocalHost, mServer.port());
        QCORO_COMPARE(socket.state(), QAbstractSocket::ConnectedState);

        socket.write("GET /stream HTTP/1.1\r\n");

        QCORO_TEST_IODEVICE_READALL(socket);
        QCORO_VERIFY(mServer.waitForConnection());
    }

    void testThenReadAllTriggers_coro(TestLoop &el) {
        QTcpSocket socket;
        bool called = false;
        qCoro(socket).connectToHost(QHostAddress::LocalHost, mServer.port()).then([&](bool connected) {
            if (!connected) {
                el.quit();
            }
            QVERIFY(connected);
            QCOMPARE(socket.state(), QAbstractSocket::ConnectedState);

            socket.write("GET /block HTTP/1.1\r\n");

            QByteArray data;
            qCoro(socket).readAll().then([&](const QByteArray &data) {
                el.quit();
                called = true;
                QVERIFY(!data.isEmpty());
            });
        });
        el.exec();

        QVERIFY(called);
        QVERIFY(mServer.waitForConnection());
    }

    QCoro::Task<> testReadTriggers_coro(QCoro::TestContext) {
        QTcpSocket socket;
        co_await qCoro(socket).connectToHost(QHostAddress::LocalHost, mServer.port());
        QCORO_COMPARE(socket.state(), QAbstractSocket::ConnectedState);

        socket.write("GET /stream HTTP/1.1\r\n");

        QCORO_TEST_IODEVICE_READ(socket);
        QCORO_VERIFY(mServer.waitForConnection());
    }

     void testThenReadTriggers_coro(TestLoop &el) {
        QTcpSocket socket;
        bool called = false;
        qCoro(socket).connectToHost(QHostAddress::LocalHost, mServer.port()).then([&](bool connected) {
            if (!connected) {
                el.quit();
            }
            QVERIFY(connected);
            QCOMPARE(socket.state(), QAbstractSocket::ConnectedState);

            socket.write("GET /block HTTP/1.1\r\n");

            QByteArray data;
            qCoro(socket).read(1).then([&](const QByteArray &data) {
                el.quit();
                called = true;
                QCOMPARE(data.size(), 1);
            });
        });
        el.exec();

        QVERIFY(called);
        QVERIFY(mServer.waitForConnection());
    }

    QCoro::Task<> testReadLineTriggers_coro(QCoro::TestContext) {
        QTcpSocket socket;
        co_await qCoro(socket).connectToHost(QHostAddress::LocalHost, mServer.port());
        QCORO_COMPARE(socket.state(), QAbstractSocket::ConnectedState);

        socket.write("GET /stream HTTP/1.1\r\n");

        QCORO_TEST_IODEVICE_READLINE(socket);
        QCORO_COMPARE(lines.size(), 14);
        QCORO_VERIFY(mServer.waitForConnection());
    }

     void testThenReadLineTriggers_coro(TestLoop &el) {
        QTcpSocket socket;
        bool called = false;
        qCoro(socket).connectToHost(QHostAddress::LocalHost, mServer.port()).then([&](bool connected) {
            if (!connected) {
                el.quit();
            }
            QVERIFY(connected);
            QCOMPARE(socket.state(), QAbstractSocket::ConnectedState);

            socket.write("GET /stream HTTP/1.1\r\n");

            QByteArray data;
            qCoro(socket).readLine().then([&](const QByteArray &data) {
                el.quit();
                called = true;
                QVERIFY(!data.isEmpty());
            });
        });
        el.exec();

        QVERIFY(called);
        QVERIFY(mServer.waitForConnection());
    }

private Q_SLOTS:
    void init() {
        mServer.start(QHostAddress::LocalHost);
    }

    void cleanup() {
        mServer.stop();
    }

    addCoroAndThenTests(WaitForConnectedTriggers)
    addCoroAndThenTests(WaitForConnectedTimeout)
    addCoroAndThenTests(WaitForDisconnectedTriggers)
    addCoroAndThenTests(WaitForDisconnectedTimeout)
    addCoroAndThenTests(DoesntCoAwaitConnectedSocket)
    addCoroAndThenTests(DoesntCoAwaitDisconnectedSocket)
    addCoroAndThenTests(ConnectToServerWithArgs)
    addCoroAndThenTests(ReadAllTriggers)
    addCoroAndThenTests(ReadTriggers)
    addCoroAndThenTests(ReadLineTriggers)

private:
    TestHttpServer<QTcpServer> mServer;
};

QTEST_GUILESS_MAIN(QCoroAbstractSocketTest)

#include "qcoroabstractsocket.moc"
