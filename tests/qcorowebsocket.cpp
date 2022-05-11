// SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "testobject.h"
#include "testwsserver.h"
#include "qcoro/websockets/qcorowebsocket.h"

#include <QWebSocket>

class QCoroWebSocketTest : public QCoro::TestObject<QCoroWebSocketTest> {
    Q_OBJECT

private:
    QCoro::Task<> testWaitForOpenWithUrl_coro(QCoro::TestContext) {
        QWebSocket socket;
        const auto result = co_await qCoro(socket).open(mServer.url());
        QCORO_VERIFY(result);
        QCORO_COMPARE(socket.state(), QAbstractSocket::ConnectedState);
        QCORO_VERIFY(mServer.waitForConnection());
    }

    void testThenWaitForOpenWithUrl_coro(TestLoop &el) {
        QWebSocket socket;
        bool called = false;
        qCoro(socket).open(mServer.url()).then([&el, &called](bool connected) {
            called = true;
            el.quit();
            QVERIFY(connected);
        });
        el.exec();
        QCOMPARE(socket.state(), QAbstractSocket::ConnectedState);
        QVERIFY(called);
        QVERIFY(mServer.waitForConnection());
    }

    QCoro::Task<> testTimeoutOpenWithUrl_coro(QCoro::TestContext) {
        QWebSocket socket;
        const auto url = mServer.url();
        mServer.stop(); // stop the server so we cannot connect

        const auto result = co_await qCoro(socket).open(url, 10ms);
        QCORO_VERIFY(!result);
    }

    void testThenTimeoutOpenWithUrl_coro(TestLoop &el) {
        QWebSocket socket;
        const auto url = mServer.url();
        mServer.stop();
        bool called = false;
        qCoro(socket).open(url, 10ms).then([&el, &called](bool connected) {
            el.quit();
            called = true;
            QVERIFY(!connected);
        });
        el.exec();
        QVERIFY(called);
    }

    QCoro::Task<> testWaitForOpenWithNetworkRequest_coro(QCoro::TestContext) {
        QWebSocket socket;
        QNetworkRequest request(mServer.url());
        const auto result = co_await qCoro(socket).open(request);
        QCORO_VERIFY(result);
        QCORO_COMPARE(socket.state(), QAbstractSocket::ConnectedState);
        QCORO_VERIFY(mServer.waitForConnection());
    }

    void testThenWaitForOpenWithNetworkRequest_coro(TestLoop &el) {
        QWebSocket socket;
        bool called = false;
        qCoro(socket).open(QNetworkRequest{mServer.url()}).then([&el, &called](bool connected) {
            called = true;
            el.quit();
            QVERIFY(connected);
        });
        el.exec();
        QCOMPARE(socket.state(), QAbstractSocket::ConnectedState);
        QVERIFY(called);
        QVERIFY(mServer.waitForConnection());
    }

    QCoro::Task<> testDoesntCoawaitOpenedSocket_coro(QCoro::TestContext ctx) {
        QWebSocket socket;
        QCORO_VERIFY(connectSocket(socket));

        ctx.setShouldNotSuspend();

        QCORO_COMPARE(socket.state(), QAbstractSocket::ConnectedState);
        const auto connected = co_await qCoro(socket).open(mServer.url());
        QCORO_VERIFY(connected);
    }

    QCoro::Task<> testPing_coro(QCoro::TestContext) {
        QWebSocket socket;
        QCORO_VERIFY(connectSocket(socket));

        const auto response = co_await qCoro(socket).ping("PING!");
        QCORO_VERIFY(response.has_value());
        QCORO_VERIFY(*response >= 0); // the latency will be somewhere around 0
    }

    void testThenPing_coro(TestLoop &el) {
        QWebSocket socket;
        QVERIFY(connectSocket(socket));
        bool called = false;
        qCoro(socket).ping("PING!").then([&el, &called](std::optional<qint64> pong) {
            el.quit();
            called = true;
            QVERIFY(pong.has_value());
            QVERIFY(*pong >= 0);
        });
        el.exec();
        QVERIFY(called);
    }

    QCoro::Task<> testBinaryFrame_coro(QCoro::TestContext) {
        QWebSocket socket;
        QCORO_VERIFY(connectSocket(socket));

        QCORO_DELAY(socket.sendBinaryMessage("TEST MESSAGE"));
        const auto frame = co_await qCoro(socket).binaryFrame();
        QCORO_VERIFY(frame.has_value());
        QCORO_COMPARE(std::get<0>(*frame), QByteArray("TEST MESSAGE"));
        QCORO_COMPARE(std::get<1>(*frame), true);
    }

    void testThenBinaryFrame_coro(TestLoop &el) {
        QWebSocket socket;
        QVERIFY(connectSocket(socket));

        QCORO_DELAY(socket.sendBinaryMessage("TEST MESSAGE"));
        bool called = false;
        qCoro(socket).binaryFrame().then([&el, &called](const auto &frame) {
            el.quit();
            called = true;
            QVERIFY(frame.has_value());
            QCOMPARE(std::get<0>(*frame), QByteArray("TEST MESSAGE"));
            QCOMPARE(std::get<1>(*frame), true);
        });
        el.exec();
        QVERIFY(called);
    }

    QCoro::Task<> testBinaryFrameTimeout_coro(QCoro::TestContext) {
        mServer.setExpectTimeout();

        QWebSocket socket;
        QCORO_VERIFY(connectSocket(socket));

        const auto frame = co_await qCoro(socket).binaryFrame(10ms);
        QCORO_VERIFY(!frame.has_value());
    }

    void testThenBinaryFrameTimeout_coro(TestLoop &el) {
        mServer.setExpectTimeout();

        QWebSocket socket;
        QVERIFY(connectSocket(socket));

        bool called = false;
        qCoro(socket).binaryFrame(10ms).then([&el, &called](const auto &frame) {
            el.quit();
            called = true;
            QVERIFY(!frame.has_value());
        });
        el.exec();
        QVERIFY(called);
    }

    QCoro::Task<> testBinaryMessage_coro(QCoro::TestContext) {
        QWebSocket socket;
        QCORO_VERIFY(connectSocket(socket));

        QCORO_DELAY(socket.sendBinaryMessage("TEST MESSAGE"));
        const auto message = co_await qCoro(socket).binaryMessage();
        QCORO_VERIFY(message.has_value());
        QCORO_COMPARE(*message, QByteArray("TEST MESSAGE"));
    }

    void testThenBinaryMessage_coro(TestLoop &el) {
        QWebSocket socket;
        QVERIFY(connectSocket(socket));

        QCORO_DELAY(socket.sendBinaryMessage("TEST MESSAGE"));
        bool called = false;
        qCoro(socket).binaryMessage().then([&el, &called](const auto &msg) {
            el.quit();
            called = true;
            QVERIFY(msg.has_value());
            QCOMPARE(*msg, QByteArray("TEST MESSAGE"));
        });
        el.exec();
        QVERIFY(called);
    }

    QCoro::Task<> testBinaryMessageTimeout_coro(QCoro::TestContext) {
        mServer.setExpectTimeout();

        QWebSocket socket;
        QCORO_VERIFY(connectSocket(socket));

        const auto message = co_await qCoro(socket).binaryMessage(10ms);
        QCORO_VERIFY(!message.has_value());
    }

    void testThenBinaryMessageTimeout_coro(TestLoop &el) {
        mServer.setExpectTimeout();

        QWebSocket socket;
        QVERIFY(connectSocket(socket));

        bool called = false;
        qCoro(socket).binaryMessage(10ms).then([&el, &called](const auto &msg) {
            el.quit();
            called = true;
            QVERIFY(!msg.has_value());
        });
        el.exec();
        QVERIFY(called);
    }

    QCoro::Task<> testTextFrame_coro(QCoro::TestContext) {
        QWebSocket socket;
        QCORO_VERIFY(connectSocket(socket));

        QCORO_DELAY(socket.sendTextMessage(QStringLiteral("TEST MESSAGE")));
        const auto frame = co_await qCoro(socket).textFrame();
        QCORO_VERIFY(frame.has_value());
        QCORO_COMPARE(std::get<0>(*frame), QStringLiteral("TEST MESSAGE"));
        QCORO_COMPARE(std::get<1>(*frame), true);
    }

    void testThenTextFrame_coro(TestLoop &el) {
        QWebSocket socket;
        QVERIFY(connectSocket(socket));

        QCORO_DELAY(socket.sendTextMessage(QStringLiteral("TEST MESSAGE")));
        bool called = false;
        qCoro(socket).textFrame().then([&el, &called](const auto &frame) {
            el.quit();
            called = true;
            QVERIFY(frame.has_value());
            QCOMPARE(std::get<0>(*frame), QStringLiteral("TEST MESSAGE"));
            QCOMPARE(std::get<1>(*frame), true);
        });
        el.exec();
        QVERIFY(called);
    }

    QCoro::Task<> testTextFrameTimeout_coro(QCoro::TestContext) {
        mServer.setExpectTimeout();

        QWebSocket socket;
        QCORO_VERIFY(connectSocket(socket));

        const auto frame = co_await qCoro(socket).textFrame(10ms);
        QCORO_VERIFY(!frame.has_value());
    }

    void testThenTextFrameTimeout_coro(TestLoop &el) {
        mServer.setExpectTimeout();

        QWebSocket socket;
        QVERIFY(connectSocket(socket));

        bool called = false;
        qCoro(socket).textFrame(10ms).then([&el, &called](const auto &frame) {
            el.quit();
            called = true;
            QVERIFY(!frame.has_value());
        });
        el.exec();
        QVERIFY(called);
    }

    QCoro::Task<> testTextMessage_coro(QCoro::TestContext) {
        QWebSocket socket;
        QCORO_VERIFY(connectSocket(socket));

        QCORO_DELAY(socket.sendTextMessage(QStringLiteral("TEST MESSAGE")));
        const auto message = co_await qCoro(socket).textMessage();
        QCORO_VERIFY(message.has_value());
        QCORO_COMPARE(*message, QStringLiteral("TEST MESSAGE"));
    }

    void testThenTextMessage_coro(TestLoop &el) {
        QWebSocket socket;
        QVERIFY(connectSocket(socket));

        QCORO_DELAY(socket.sendTextMessage(QStringLiteral("TEST MESSAGE")));
        bool called = false;
        qCoro(socket).textMessage().then([&el, &called](const auto &msg) {
            el.quit();
            called = true;
            QVERIFY(msg.has_value());
            QCOMPARE(*msg, QStringLiteral("TEST MESSAGE"));
        });
        el.exec();
        QVERIFY(called);
    }

    QCoro::Task<> testTextMessageTimeout_coro(QCoro::TestContext) {
        mServer.setExpectTimeout();

        QWebSocket socket;
        QCORO_VERIFY(connectSocket(socket));

        const auto message = co_await qCoro(socket).textMessage(10ms);
        QCORO_VERIFY(!message.has_value());
    }

    void testThenTextMessageTimeout_coro(TestLoop &el) {
        mServer.setExpectTimeout();

        QWebSocket socket;
        QVERIFY(connectSocket(socket));

        bool called = false;
        qCoro(socket).textMessage(10ms).then([&el, &called](const auto &msg) {
            el.quit();
            called = true;
            QVERIFY(!msg.has_value());
        });
        el.exec();
        QVERIFY(called);
    }

    QCoro::Task<> testReadFragmentedMessage_coro(QCoro::TestContext) {
        QWebSocket socket;
        connect(&socket, &QWebSocket::binaryFrameReceived, this,
                [](const QByteArray &frame, bool isLast) {
                    qDebug() << "SLOT RECEIVED FRAME " << frame.size() << isLast;
                });
        QUrl url = mServer.url();
        url.setPath(QStringLiteral("/large"));
        QCORO_VERIFY(QCoro::waitFor(qCoro(socket).open(url)));

        QCORO_DELAY(socket.sendBinaryMessage("One large, please"));

        auto coroSocket = qCoro(socket);

        QByteArray data;
        Q_FOREVER {
            const auto response = co_await coroSocket.binaryFrame();
            QCORO_VERIFY(response.has_value());
            qDebug() << std::get<0>(*response).size() << std::get<1>(*response);
            data += std::get<0>(*response);
            if (std::get<1>(*response)) { // last
                break;
            }
        }

        qDebug() << data.size();
        QCORO_VERIFY(data.size() >= 10 * 1024 * 1024); // 10MB
    }

private Q_SLOTS:
    void init() {
        mServer.start();
    }

    void cleanup() {
        mServer.stop();
    }

    addCoroAndThenTests(WaitForOpenWithUrl)
    addCoroAndThenTests(TimeoutOpenWithUrl)
    addCoroAndThenTests(WaitForOpenWithNetworkRequest)
    addTest(DoesntCoawaitOpenedSocket)
    addCoroAndThenTests(Ping)
    addCoroAndThenTests(BinaryFrame)
    addCoroAndThenTests(BinaryFrameTimeout)
    addCoroAndThenTests(BinaryMessage)
    addCoroAndThenTests(BinaryMessageTimeout)
    addCoroAndThenTests(TextFrame)
    addCoroAndThenTests(TextFrameTimeout)
    addCoroAndThenTests(TextMessage)
    addCoroAndThenTests(TextMessageTimeout)

    addTest(ReadFragmentedMessage)

private:
    bool connectSocket(QWebSocket &socket) {
        return QCoro::waitFor(qCoro(socket).open(mServer.url()));
    }

    TestWsServer mServer;
};

QTEST_GUILESS_MAIN(QCoroWebSocketTest)

#include "qcorowebsocket.moc"
