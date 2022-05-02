// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "testhttpserver.h"
#include "testobject.h"
#include "qcoroiodevice_macros.h"
#include "testloop.h"

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
        QCORO_VERIFY(mServer.waitForConnection());
    }

    void testThenWaitForConnectedTriggers_coro(TestLoop &el) {
        QLocalSocket socket;
        QCORO_DELAY(socket.connectToServer(QCoroLocalSocketTest::getSocketName()));
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
        QLocalSocket socket;
        socket.connectToServer(QCoroLocalSocketTest::getSocketName());
        QCORO_COMPARE(socket.state(), QLocalSocket::ConnectedState);

        QCORO_DELAY(socket.disconnectFromServer());

        co_await qCoro(socket).waitForDisconnected();

        QCORO_COMPARE(socket.state(), QLocalSocket::UnconnectedState);
        QCORO_VERIFY(mServer.waitForConnection());
    }

    void testThenWaitForDisconnectedTriggers_coro(TestLoop &el) {
        QLocalSocket socket;
        socket.connectToServer(QCoroLocalSocketTest::getSocketName());
        QCOMPARE(socket.state(), QLocalSocket::ConnectedState);

        QCORO_DELAY(socket.disconnectFromServer());
        bool called = false;
        qCoro(socket).waitForDisconnected().then([&](bool disconnected) {
            called = true;
            el.quit();
            QVERIFY(disconnected);
        });
        el.exec();
        QVERIFY(called);
        QVERIFY(mServer.waitForConnection());
    }

    // On Linux at least, QLocalSocket connects immediately and synchronously
    QCoro::Task<> testDoesntCoAwaitConnectedSocket_coro(QCoro::TestContext context) {
        context.setShouldNotSuspend();

        QLocalSocket socket;
        socket.connectToServer(QCoroLocalSocketTest::getSocketName());

        QCORO_COMPARE(socket.state(), QLocalSocket::ConnectedState);

        co_await qCoro(socket).waitForConnected();
        QCORO_VERIFY(mServer.waitForConnection());
    }

    void testThenDoesntCoAwaitConnectedSocket_coro(TestLoop &el) {
        QLocalSocket socket;
        socket.connectToServer(QCoroLocalSocketTest::getSocketName());
        QCOMPARE(socket.state(), QLocalSocket::ConnectedState);

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

    QCoro::Task<> testDoesntCoAwaitDisconnectedSocket_coro(QCoro::TestContext context) {
        context.setShouldNotSuspend();
        mServer.setExpectTimeout(true);

        QLocalSocket socket;
        QCORO_COMPARE(socket.state(), QLocalSocket::UnconnectedState);

        co_await qCoro(socket).waitForDisconnected();
    }

    void testThenDoesntCoAwaitDisconnectedSocket_coro(TestLoop &el) {
        mServer.setExpectTimeout(true);

        QLocalSocket socket;
        QCOMPARE(socket.state(), QLocalSocket::UnconnectedState);
        bool called = false;
        qCoro(socket).waitForDisconnected().then([&](bool disconnected) {
            called = true;
            el.quit();
            QVERIFY(!disconnected);
        });
        el.exec();
        QVERIFY(called);
    }

    QCoro::Task<> testConnectToServerWithArgs_coro(QCoro::TestContext context) {
        context.setShouldNotSuspend();

        QLocalSocket socket;

        co_await qCoro(socket).connectToServer(QCoroLocalSocketTest::getSocketName());

        QCORO_COMPARE(socket.state(), QLocalSocket::ConnectedState);
        QCORO_VERIFY(mServer.waitForConnection());
    }

    void testThenConnectToServerWithArgs_coro(TestLoop &el) {
        QLocalSocket socket;
        bool called = false;
        qCoro(socket).connectToServer(QCoroLocalSocketTest::getSocketName()).then([&](bool connected) {
            called = true;
            el.quit();
            QVERIFY(connected);
        });
        el.exec();
        QVERIFY(called);
        QVERIFY(mServer.waitForConnection());
    }

    QCoro::Task<> testConnectToServer_coro(QCoro::TestContext context) {
        context.setShouldNotSuspend();

        QLocalSocket socket;
        socket.setServerName(QCoroLocalSocketTest::getSocketName());

        co_await qCoro(socket).connectToServer();

        QCORO_COMPARE(socket.state(), QLocalSocket::ConnectedState);
        QCORO_VERIFY(mServer.waitForConnection());
    }

    void testThenConnectToServer_coro(TestLoop &el) {
        QLocalSocket socket;
        socket.setServerName(QCoroLocalSocketTest::getSocketName());
        bool called = false;
        qCoro(socket).connectToServer().then([&](bool connected) {
            called = true;
            el.quit();
            QVERIFY(connected);
        });
        el.exec();
        QVERIFY(called);
        QVERIFY(mServer.waitForConnection());
    }

    QCoro::Task<> testWaitForConnectedTimeout_coro(QCoro::TestContext) {
        mServer.setExpectTimeout(true);

        QLocalSocket socket;

        QCORO_TEST_TIMEOUT(co_await qCoro(socket).waitForConnected(10ms));
    }

    void testThenWaitForConnectedTimeout_coro(TestLoop &el) {
        mServer.setExpectTimeout(true);

        QLocalSocket socket;

        bool called = false;
        qCoro(socket).waitForConnected(10ms).then([&](bool connected) {
            called = true;
            el.quit();
            QVERIFY(!connected);
        });
        const auto start = std::chrono::steady_clock::now();
        el.exec();
        const auto end = std::chrono::steady_clock::now();
        QVERIFY(end - start < 500ms);
        QVERIFY(called);
    }

    QCoro::Task<> testWaitForDisconnectedTimeout_coro(QCoro::TestContext) {
        QLocalSocket socket;
        socket.connectToServer(QCoroLocalSocketTest::getSocketName());
        QCORO_COMPARE(socket.state(), QLocalSocket::ConnectedState);

        QCORO_TEST_TIMEOUT(co_await qCoro(socket).waitForDisconnected(10ms));
        QCORO_VERIFY(mServer.waitForConnection());
    }

    void testThenWaitForDisconnectedTimeout_coro(TestLoop &el) {
        QLocalSocket socket;
        socket.connectToServer(QCoroLocalSocketTest::getSocketName());
        QCOMPARE(socket.state(), QLocalSocket::ConnectedState);
        bool called = false;
        qCoro(socket).waitForDisconnected(10ms).then([&](bool disconnected) {
            called = true;
            el.quit();
            QVERIFY(!disconnected);
        });
        const auto start = std::chrono::steady_clock::now();
        el.exec();
        const auto end = std::chrono::steady_clock::now();
        QVERIFY(end - start < 500ms);
        QVERIFY(called);
        QVERIFY(mServer.waitForConnection());
    }

    QCoro::Task<> testReadAllTriggers_coro(QCoro::TestContext) {
        QLocalSocket socket;
        socket.connectToServer(QCoroLocalSocketTest::getSocketName());
        QCORO_COMPARE(socket.state(), QLocalSocket::ConnectedState);

        socket.write("GET /stream HTTP/1.1\r\n");

        QCORO_TEST_IODEVICE_READALL(socket);
        QCORO_VERIFY(mServer.waitForConnection());
    }

    void testThenReadAllTriggers_coro(TestLoop &el) {
        QLocalSocket socket;
        socket.connectToServer(QCoroLocalSocketTest::getSocketName());
        QCOMPARE(socket.state(), QLocalSocket::ConnectedState);
        bool called = false;
        qCoro(socket).readAll().then([&](const QByteArray &data) {
            called = true;
            el.quit();
            QVERIFY(!data.isEmpty());
        });
        socket.write("GET /block HTTP/1.1\r\n");
        el.exec();

        QVERIFY(called);
        QVERIFY(mServer.waitForConnection());
    }

    QCoro::Task<> testReadTriggers_coro(QCoro::TestContext) {
        QLocalSocket socket;
        socket.connectToServer(QCoroLocalSocketTest::getSocketName());
        QCORO_COMPARE(socket.state(), QLocalSocket::ConnectedState);

        socket.write("GET /stream HTTP/1.1\r\n");

        QCORO_TEST_IODEVICE_READ(socket);
        QCORO_VERIFY(mServer.waitForConnection());
    }

    void testThenReadTriggers_coro(TestLoop &el) {
        QLocalSocket socket;
        socket.connectToServer(QCoroLocalSocketTest::getSocketName());
        QCOMPARE(socket.state(), QLocalSocket::ConnectedState);
        bool called = false;
        qCoro(socket).read(1).then([&](const QByteArray &data) {
            called = true;
            el.quit();
            QCOMPARE(data.size(), 1);
        });
        socket.write("GET /block HTTP/1.1\r\n");
        el.exec();
        QVERIFY(called);
        QVERIFY(mServer.waitForConnection());
    }

    QCoro::Task<> testReadLineTriggers_coro(QCoro::TestContext) {
        QLocalSocket socket;
        socket.connectToServer(QCoroLocalSocketTest::getSocketName());
        QCORO_COMPARE(socket.state(), QLocalSocket::ConnectedState);

        socket.write("GET /stream HTTP/1.1\r\n");

        QCORO_TEST_IODEVICE_READLINE(socket);
        QCORO_COMPARE(lines.size(), 14);
        QCORO_VERIFY(mServer.waitForConnection());
    }

    void testThenReadLineTriggers_coro(TestLoop &el) {
        QLocalSocket socket;
        socket.connectToServer(QCoroLocalSocketTest::getSocketName());
        QCOMPARE(socket.state(), QLocalSocket::ConnectedState);
        bool called = false;
        qCoro(socket).readLine().then([&](const QByteArray &data) {
            called = true;
            el.quit();
            QVERIFY(!data.isEmpty());
        });
        socket.write("GET /block HTTP/1.1\r\n");
        el.exec();

        QVERIFY(called);
        QVERIFY(mServer.waitForConnection());
    }

private Q_SLOTS:
    void init() {
        mServer.start(QCoroLocalSocketTest::getSocketName());
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
    addCoroAndThenTests(ConnectToServer)
    addCoroAndThenTests(ReadAllTriggers)
    addCoroAndThenTests(ReadTriggers)
    addCoroAndThenTests(ReadLineTriggers)

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
