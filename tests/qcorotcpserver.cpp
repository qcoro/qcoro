// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "testobject.h"

#include "qcoro/network/qcorotcpserver.h"
#include "qcoro/network/qcoroabstractsocket.h"

#include <QTcpServer>
#include <QTcpSocket>

#include <thread>
#include <mutex>

using namespace std::chrono_literals;

class Client {
public:
    Client(uint16_t serverPort, std::mutex &mutex, bool &ok)
        : mThread([serverPort, &mutex, &ok]() mutable {
            std::this_thread::sleep_for(500ms);

            std::lock_guard lock{mutex};
            QTcpSocket socket;
            socket.connectToHost(QHostAddress::LocalHost, serverPort);
            if (!socket.waitForConnected(10'000)) {
                qWarning() << "Not connected within timeout" << socket.errorString();
                ok = false;
                return;
            }
            socket.write("Hello World!");
            socket.flush();
            socket.close();
            ok = true;
        })
    {}

    ~Client() {
        mThread.join();
    }

private:
    std::thread mThread;
};

class QCoroTcpServerTest: public QCoro::TestObject<QCoroTcpServerTest> {
    Q_OBJECT

private:
    QCoro::Task<> testWaitForNewConnectionTriggers_coro(QCoro::TestContext) {
        QTcpServer server;
        QCORO_VERIFY(server.listen(QHostAddress::LocalHost));
        QCORO_VERIFY(server.isListening());
        const quint16 serverPort = server.serverPort();

        std::mutex mutex;
        bool ok = false;
        Client client(serverPort, mutex, ok);

        auto *connection = co_await qCoro(server).waitForNewConnection(10s);
        QCORO_VERIFY(connection != nullptr);
        const auto data = co_await qCoro(connection).readAll();
        QCORO_COMPARE(data, QByteArray{"Hello World!"});

        std::lock_guard lock{mutex};
        QCORO_VERIFY(ok);
    }

    void testThenWaitForNewConnectionTriggers_coro(TestLoop &el) {
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost));
        const quint16 serverPort = server.serverPort();

        std::mutex mutex;
        bool ok = false;
        Client client(serverPort, mutex, ok);

        bool called = false;
        qCoro(server).waitForNewConnection(10s).then([&](QTcpSocket *socket) -> QCoro::Task<void> {
            called = true;
            if (!socket) {
                el.quit();
                co_return;
            }

            const auto data = co_await qCoro(socket).readAll();
            QCORO_COMPARE(data, QByteArray("Hello World!"));
            el.quit();
        });
        el.exec();

        std::lock_guard lock{mutex};
        QVERIFY(called);
        QVERIFY(ok);
    }

    QCoro::Task<> testDoesntCoAwaitPendingConnection_coro(QCoro::TestContext testContext) {
        testContext.setShouldNotSuspend();

        QTcpServer server;
        QCORO_VERIFY(server.listen(QHostAddress::LocalHost));
        const int serverPort = server.serverPort();

        bool ok = false;
        std::mutex mutex;
        Client client(serverPort, mutex, ok);

        QCORO_VERIFY(server.waitForNewConnection(10'000));

        auto *connection = co_await qCoro(server).waitForNewConnection(10s);

        connection->waitForReadyRead(); // can't use coroutine, it might suspend or not, depending on how eventloop
                                        // gets triggered, which fails the test since it's setShouldNotSuspend()
        QCORO_COMPARE(connection->readAll(), QByteArray{"Hello World!"});

        std::lock_guard lock{mutex};
        QCORO_VERIFY(ok);
    }

private Q_SLOTS:
    addCoroAndThenTests(WaitForNewConnectionTriggers)
    addTest(DoesntCoAwaitPendingConnection)
};

QTEST_GUILESS_MAIN(QCoroTcpServerTest)

#include "qcorotcpserver.moc"
