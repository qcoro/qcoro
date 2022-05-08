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

private Q_SLOTS:
    void init() {
        mServer.start();
    }

    void cleanup() {
        mServer.stop();
    }

    addCoroAndThenTests(WaitForOpenWithUrl)
    addCoroAndThenTests(WaitForOpenWithNetworkRequest)

private:
    TestWsServer mServer;
};

QTEST_GUILESS_MAIN(QCoroWebSocketTest)

#include "qcorowebsocket.moc"