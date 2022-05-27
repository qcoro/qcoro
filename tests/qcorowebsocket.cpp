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
        auto frames = qCoro(socket).binaryFrames();
        const auto frame = co_await frames.begin();
        QCORO_VERIFY(frame != frames.end());
        QCORO_COMPARE(std::get<0>(*frame), QByteArray("TEST MESSAGE"));
        QCORO_COMPARE(std::get<1>(*frame), true);
    }

    QCoro::Task<> testBinaryFrameTimeout_coro(QCoro::TestContext) {
        mServer.setExpectTimeout();

        QWebSocket socket;
        QCORO_VERIFY(connectSocket(socket));

        auto frames = qCoro(socket).binaryFrames(10ms);
        const auto frame = co_await frames.begin();
        QCORO_COMPARE(frame, frames.end());
    }

    QCoro::Task<> testBinaryMessage_coro(QCoro::TestContext) {
        QWebSocket socket;
        QCORO_VERIFY(connectSocket(socket));

        QCORO_DELAY(socket.sendBinaryMessage("TEST MESSAGE"));
        auto messages = qCoro(socket).binaryMessages();
        const auto message = co_await messages.begin();
        QCORO_VERIFY(message != messages.end());
        QCORO_COMPARE(*message, QByteArray("TEST MESSAGE"));
    }

    QCoro::Task<> testBinaryMessageTimeout_coro(QCoro::TestContext) {
        mServer.setExpectTimeout();

        QWebSocket socket;
        QCORO_VERIFY(connectSocket(socket));

        auto messages = qCoro(socket).binaryMessages(10ms);
        const auto message = co_await messages.begin();
        QCORO_COMPARE(message, messages.end());
    }

    QCoro::Task<> testTextFrame_coro(QCoro::TestContext) {
        QWebSocket socket;
        QCORO_VERIFY(connectSocket(socket));

        QCORO_DELAY(socket.sendTextMessage(QStringLiteral("TEST MESSAGE")));
        auto frames = qCoro(socket).textFrames();
        const auto frame = co_await frames.begin();
        QCORO_VERIFY(frame != frames.end());
        QCORO_COMPARE(std::get<0>(*frame), QStringLiteral("TEST MESSAGE"));
        QCORO_COMPARE(std::get<1>(*frame), true);
    }

    QCoro::Task<> testTextFrameTimeout_coro(QCoro::TestContext) {
        mServer.setExpectTimeout();

        QWebSocket socket;
        QCORO_VERIFY(connectSocket(socket));

        auto frames = qCoro(socket).textFrames(10ms);
        const auto frame = co_await frames.begin();
        QCORO_COMPARE(frame, frames.end());
    }

    QCoro::Task<> testTextMessage_coro(QCoro::TestContext) {
        QWebSocket socket;
        QCORO_VERIFY(connectSocket(socket));

        QCORO_DELAY(socket.sendTextMessage(QStringLiteral("TEST MESSAGE")));
        auto messages = qCoro(socket).textMessages();
        const auto message = co_await messages.begin();
        QCORO_VERIFY(message != messages.end());
        QCORO_COMPARE(*message, QStringLiteral("TEST MESSAGE"));
    }

    QCoro::Task<> testTextMessageTimeout_coro(QCoro::TestContext) {
        mServer.setExpectTimeout();

        QWebSocket socket;
        QCORO_VERIFY(connectSocket(socket));

        auto messages = qCoro(socket).textMessages(10ms);
        const auto message = co_await messages.begin();
        QCORO_COMPARE(message, messages.end());
    }

    QCoro::Task<> testReadFragmentedMessage_coro(QCoro::TestContext) {
        QWebSocket socket;
        QUrl url = mServer.url();
        url.setPath(QStringLiteral("/large"));
        QCORO_VERIFY(QCoro::waitFor(qCoro(socket).open(url)));

        QCORO_DELAY(socket.sendBinaryMessage("One large, please"));

        auto frames = qCoro(socket).binaryFrames();
        QByteArray data;
        for (auto frame = co_await frames.begin(), end = frames.end(); frame != end; co_await ++frame) {
            data += std::get<0>(*frame);
            if (std::get<1>(*frame)) { // last
                break;
            }
        }

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
    addTest(BinaryFrame)
    addTest(BinaryFrameTimeout)
    addTest(BinaryMessage)
    addTest(BinaryMessageTimeout)
    addTest(TextFrame)
    addTest(TextFrameTimeout)
    addTest(TextMessage)
    addTest(TextMessageTimeout)

    addTest(ReadFragmentedMessage)

private:
    bool connectSocket(QWebSocket &socket) {
        return QCoro::waitFor(qCoro(socket).open(mServer.url()));
    }

    TestWsServer mServer;
};

QTEST_GUILESS_MAIN(QCoroWebSocketTest)

#include "qcorowebsocket.moc"
