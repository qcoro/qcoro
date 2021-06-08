// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "testobject.h"
#include "qcoro/qcorotcpserver.h"
#include "qcoro/qcoroiodevice.h"

#include <QTcpServer>
#include <QTcpSocket>

#include <thread>

using namespace std::chrono_literals;

class QCoroTcpServerTest: public QCoro::TestObject<QCoroTcpServerTest> {
    Q_OBJECT

private:
    QCoro::Task<> testWaitForNewConnectionTriggers_coro(QCoro::TestContext) {
        QTcpServer server;
        QCORO_VERIFY(server.listen(QHostAddress::LocalHost, 45678));

        bool ok = false;
        std::thread clientThread{[&ok]() mutable {
            std::this_thread::sleep_for(500ms);

            QTcpSocket socket;
            socket.connectToHost(QHostAddress::LocalHost, 45678);
            if (!socket.waitForConnected(10'000)) {
                ok = false;
                return;
            }

            socket.write("Hello World!");
            socket.flush();
            socket.close();
            ok = true;
        }};

        auto *connection = co_await qCoro(server).waitForNewConnection(10s);
        QCORO_VERIFY(connection != nullptr);

        const auto data = co_await qCoro(connection).readAll();
        QCORO_COMPARE(data, QByteArray{"Hello World!"});

        QCORO_VERIFY(ok);

        clientThread.join();
    }

    QCoro::Task<> testDoesntCoAwaitPendingConnection_coro(QCoro::TestContext testContext) {
        testContext.setShouldNotSuspend();

        QTcpServer server;
        QCORO_VERIFY(server.listen(QHostAddress::LocalHost, 45678));

        bool ok = false;
        std::thread clientThread{[&]() mutable {
            QTcpSocket socket;
            socket.connectToHost(QHostAddress::LocalHost, 45678);
            if (!socket.waitForConnected(10'000)) {
                ok = false;
                return;
            }

            socket.write("Hello World!");
            socket.flush();
            socket.close();
            ok = true;
        }};

        QCORO_VERIFY(server.waitForNewConnection(10'000));

        auto *connection = co_await qCoro(server).waitForNewConnection(10s);

        connection->waitForReadyRead(); // can't use coroutine, it might suspend or not, depending on how eventloop
                                        // gets triggered, which fails the test since it's setShouldNotSuspend()
        QCORO_COMPARE(connection->readAll(), QByteArray{"Hello World!"});
        QCORO_VERIFY(ok);

        clientThread.join();
    }

private Q_SLOTS:
    addTest(WaitForNewConnectionTriggers)
    addTest(DoesntCoAwaitPendingConnection)
};

QTEST_GUILESS_MAIN(QCoroTcpServerTest)

#include "qcorotcpserver.moc"
