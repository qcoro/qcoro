// SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "websockets/qcorowebsocketserver.h"
#include "websockets/qcorowebsocket.h"
#include "testobject.h"

#include <QWebSocketServer>
#include <QWebSocket>

#include <QTest>

class QCoroWebSocketServerTest : public QCoro::TestObject<QCoroWebSocketServerTest> {
    Q_OBJECT
public:
    QCoroWebSocketServerTest(QObject *parent = nullptr)
        : QCoro::TestObject<QCoroWebSocketServerTest>(parent)
    {
        // On Windows, constructing QWebSocket for the first time takes some time
        // (most likely due to loading OpenSSL), which causes the first test to
        // time out on the CI.
        QWebSocket socket;
    }

private:
    QCoro::Task<> testNextPendingConnection_coro(QCoro::TestContext) {
        QWebSocketServer server(QStringLiteral("TestWSServer"), QWebSocketServer::NonSecureMode);
        QCORO_VERIFY(server.listen(QHostAddress::LocalHost));

        QWebSocket socket;
        QCORO_DELAY(socket.open(server.serverUrl()));
        const auto serverSocket = std::unique_ptr<QWebSocket>(co_await qCoro(server).nextPendingConnection());
        QCORO_VERIFY(serverSocket != nullptr);
    }

    void testThenNextPendingConnection_coro(TestLoop &el) {
        QWebSocketServer server(QStringLiteral("TestWSServer"), QWebSocketServer::NonSecureMode);
        QVERIFY(server.listen(QHostAddress::LocalHost));

        QWebSocket socket;
        QCORO_DELAY(socket.open(server.serverUrl()));

        bool called = false;
        qCoro(server).nextPendingConnection().then([&el, &called](QWebSocket *socket) {
            el.quit();
            called = true;
            QVERIFY(socket != nullptr);
            socket->deleteLater();
        });
        el.exec();
        QVERIFY(called);
    }

    QCoro::Task<> testNextPendingConnectionTimeout_coro(QCoro::TestContext) {
        QWebSocketServer server(QStringLiteral("TestWSServer"), QWebSocketServer::NonSecureMode);
        QCORO_VERIFY(server.listen(QHostAddress::LocalHost));

        const auto *socket = co_await qCoro(server).nextPendingConnection(10ms);
        QCORO_COMPARE(socket, nullptr);
    }

    void testThenNextPendingConnectionTimeout_coro(TestLoop &el) {
        QWebSocketServer server(QStringLiteral("TestWSServer"), QWebSocketServer::NonSecureMode);
        QVERIFY(server.listen(QHostAddress::LocalHost));

        bool called = false;
        qCoro(server).nextPendingConnection(100ms).then([&el, &called](QWebSocket *socket) {
            el.quit();
            called = true;
            QCOMPARE(socket, nullptr);
        });
        el.exec();
        QVERIFY(called);
    }

    QCoro::Task<> testClosingServerResumesAwaiters_coro(QCoro::TestContext) {
        QWebSocketServer server(QStringLiteral("TestWSServer"), QWebSocketServer::NonSecureMode);
        QCORO_VERIFY(server.listen(QHostAddress::LocalHost));
        QCORO_DELAY(server.close());

        auto *socket = co_await qCoro(server).nextPendingConnection();
        QCORO_COMPARE(socket, nullptr);
    }

    void testThenClosingServerResumesAwaiters_coro(TestLoop &el) {
        QWebSocketServer server(QStringLiteral("TestWSServer"), QWebSocketServer::NonSecureMode);
        QVERIFY(server.listen(QHostAddress::LocalHost));
        QCORO_DELAY(server.close());

        bool called = false;
        qCoro(server).nextPendingConnection().then([&el, &called](QWebSocket *socket) {
            el.quit();
            called = true;
            QCOMPARE(socket, nullptr);
        });
        el.exec();
        QVERIFY(called);
    }

    QCoro::Task<> testDoesntCoawaitNonlisteningServer_coro(QCoro::TestContext ctx) {
        ctx.setShouldNotSuspend();

        QWebSocketServer server(QStringLiteral("TestWSServer"), QWebSocketServer::NonSecureMode);

        const auto *socket = co_await qCoro(server).nextPendingConnection();
        QCORO_COMPARE(socket, nullptr);
    }

    QCoro::Task<> testDoesntCoawaitWithPendingConnection_coro(QCoro::TestContext ctx) {
        ctx.setShouldNotSuspend();

        QWebSocketServer server(QStringLiteral("TestWSServer"), QWebSocketServer::NonSecureMode);
        QCORO_VERIFY(server.listen(QHostAddress::LocalHost));

        QWebSocket socket;
        QCORO_VERIFY(QCoro::waitFor(qCoro(socket).open(server.serverUrl())));
        QCORO_COMPARE(socket.state(), QAbstractSocket::ConnectedState);

        QTest::qWait(100); // give the server time to register the incoming connection
        QCORO_VERIFY(server.hasPendingConnections());

        const auto serverSocket = std::unique_ptr<QWebSocket>(co_await qCoro(server).nextPendingConnection());
        QCORO_VERIFY(serverSocket);
    }

private Q_SLOTS:
    addCoroAndThenTests(NextPendingConnection)
    addCoroAndThenTests(NextPendingConnectionTimeout)
    addCoroAndThenTests(ClosingServerResumesAwaiters)
    addTest(DoesntCoawaitNonlisteningServer)
    addTest(DoesntCoawaitWithPendingConnection)
};

QTEST_GUILESS_MAIN(QCoroWebSocketServerTest)

#include "qcorowebsocketserver.moc"
