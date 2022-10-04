// SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "testobject.h"
#include "testwsserver.h"
#include "qcoro/websockets/qcorowebsocket.h"

#include <QWebSocket>

class QCoroWebSocketTest : public QCoro::TestObject<QCoroWebSocketTest> {
    Q_OBJECT
public:
    explicit QCoroWebSocketTest(QObject *parent = nullptr)
            : QCoro::TestObject<QCoroWebSocketTest>(parent)
    {
        // On Windows, constructing QWebSocket for the first time takes some time
        // (most likely due to loading OpenSSL), which causes the first test to
        // time out on the CI.
        QWebSocket socket;
    }
private:
    template<typename T, typename SendFunc, typename RecvFunc>
    QCoro::Task<> testReceived(const T &msg, SendFunc sendFunc, RecvFunc recvFunc) {
        QWebSocket socket;
        QCORO_VERIFY(connectSocket(socket));

        QCORO_DELAY(std::invoke(sendFunc, socket, msg));
        auto coroSocket = qCoro(socket);
        auto gen = std::invoke(recvFunc, &coroSocket, std::chrono::milliseconds{-1});
        const auto data = co_await gen.begin();
        QCORO_VERIFY(data != gen.end());
        if constexpr (std::is_same_v<std::remove_cvref_t<decltype(*data)>, QString> ||
                      std::is_same_v<std::remove_cvref_t<decltype(*data)>, QByteArray>) {
            QCORO_COMPARE(*data, msg);
        } else {
            QCORO_COMPARE(std::get<0>(*data), msg);
            QCORO_COMPARE(std::get<1>(*data), true);
        }
    }

    template<typename RecvFunc>
    QCoro::Task<> testTimeout(RecvFunc recvFunc) {
        mServer.setExpectTimeout();

        QWebSocket socket;
        QCORO_VERIFY(connectSocket(socket));

        auto coroSocket = qCoro(socket);
        auto gen = std::invoke(recvFunc, &coroSocket, 10ms);
        const auto data = co_await gen.begin();
        QCORO_COMPARE(data, gen.end());
    }

    template<typename RecvFunc>
    QCoro::Task<> testGeneratorEndOnSocketClose(RecvFunc recvFunc) {
        mServer.setExpectTimeout();

        QWebSocket socket;
        QCORO_VERIFY(connectSocket(socket));

        QCORO_DELAY(socket.close());
        auto coroSocket = qCoro(socket);
        auto gen = std::invoke(recvFunc, &coroSocket, std::chrono::milliseconds{-1});
        const auto it = co_await gen.begin();
        QCORO_COMPARE(it, gen.end());
    }

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
        QCORO_VERIFY(*response >= 0ms); // the latency will be somewhere around 0
    }

    void testThenPing_coro(TestLoop &el) {
        QWebSocket socket;
        QVERIFY(connectSocket(socket));
        bool called = false;
        qCoro(socket).ping("PING!").then([&el, &called](std::optional<std::chrono::milliseconds> pong) {
            el.quit();
            called = true;
            QVERIFY(pong.has_value());
            QVERIFY(*pong >= 0ms);
        });
        el.exec();
        QVERIFY(called);
    }

    QCoro::Task<> testBinaryFrame_coro(QCoro::TestContext) {
        co_await testReceived(QByteArray("TEST MESSAGE"), &QWebSocket::sendBinaryMessage,
                              &QCoro::detail::QCoroWebSocket::binaryFrames);
    }

    QCoro::Task<> testBinaryFrameTimeout_coro(QCoro::TestContext) {
        co_await testTimeout(&QCoro::detail::QCoroWebSocket::binaryFrames);
    }

    QCoro::Task<> testBinaryFrameGeneratorEndsOnSocketClose_coro(QCoro::TestContext) {
        co_await testGeneratorEndOnSocketClose(&QCoro::detail::QCoroWebSocket::binaryFrames);
    }

    QCoro::Task<> testBinaryMessage_coro(QCoro::TestContext) {
        co_await testReceived(QByteArray("TEST MESSAGE"), &QWebSocket::sendBinaryMessage,
                              &QCoro::detail::QCoroWebSocket::binaryMessages);
    }

    QCoro::Task<> testBinaryMessageTimeout_coro(QCoro::TestContext) {
        co_await testTimeout(&QCoro::detail::QCoroWebSocket::binaryMessages);
    }

    QCoro::Task<> testBinaryMessageGeneratorEndsOnSocketClose_coro(QCoro::TestContext) {
        co_await testGeneratorEndOnSocketClose(&QCoro::detail::QCoroWebSocket::binaryMessages);
    }

    QCoro::Task<> testTextFrame_coro(QCoro::TestContext) {
        co_await testReceived(QStringLiteral("TEST MESSAGE"), &QWebSocket::sendTextMessage,
                             &QCoro::detail::QCoroWebSocket::textFrames);
    }

    QCoro::Task<> testTextFrameTimeout_coro(QCoro::TestContext) {
        co_await testTimeout(&QCoro::detail::QCoroWebSocket::textFrames);
    }

    QCoro::Task<> testTextFrameGeneratorEndsOnSocketClose_coro(QCoro::TestContext) {
        co_await testGeneratorEndOnSocketClose(&QCoro::detail::QCoroWebSocket::textFrames);
    }

    QCoro::Task<> testTextMessage_coro(QCoro::TestContext) {
        co_await testReceived(QStringLiteral("TEST MESSAGE"), &QWebSocket::sendTextMessage,
                              &QCoro::detail::QCoroWebSocket::textMessages);
    }

    QCoro::Task<> testTextMessageTimeout_coro(QCoro::TestContext) {
        co_await testTimeout(&QCoro::detail::QCoroWebSocket::textMessages);
    }

    QCoro::Task<> testTextMessageGeneratorEndsOnSocketClose_coro(QCoro::TestContext) {
        co_await testGeneratorEndOnSocketClose(&QCoro::detail::QCoroWebSocket::textMessages);
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
    addTest(BinaryFrameGeneratorEndsOnSocketClose)
    addTest(BinaryMessage)
    addTest(BinaryMessageTimeout)
    addTest(BinaryMessageGeneratorEndsOnSocketClose)
    addTest(TextFrame)
    addTest(TextFrameTimeout)
    addTest(TextFrameGeneratorEndsOnSocketClose)
    addTest(TextMessage)
    addTest(TextMessageTimeout)
    addTest(TextMessageGeneratorEndsOnSocketClose)

    addTest(ReadFragmentedMessage)

private:
    bool connectSocket(QWebSocket &socket) {
        return QCoro::waitFor(qCoro(socket).open(mServer.url()));
    }

    TestWsServer mServer;
};

QTEST_GUILESS_MAIN(QCoroWebSocketTest)

#include "qcorowebsocket.moc"
