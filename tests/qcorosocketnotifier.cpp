// SPDX-FileCopyrightText: 2024 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "testobject.h"
#include "testhttpserver.h"

#include <QTcpServer>

#include "qcoro/network/qcorosocketnotifier.h"

class QCoroSocketNotifierTest : public QCoro::TestObject<QCoroSocketNotifierTest>
{
    Q_OBJECT
private:
    QCoro::Task<void> testNotifierActivates_coro(QCoro::TestContext) {
        QTcpSocket socket;
        socket.connectToHost(QHostAddress::LocalHost, mServer.port());
        socket.waitForConnected();

        QSocketNotifier notifier(socket.socketDescriptor(), QSocketNotifier::Read);
        socket.write("GET /block HTTP/1.1\r\n");

        co_await notifier;

        QCORO_VERIFY(socket.bytesAvailable() > 0);
    }

private Q_SLOTS:
    void init() {
        mServer.start(QHostAddress::LocalHost);
    }

    void cleanup() {
        mServer.stop();
    }

    addTest(NotifierActivates)

private:
    TestHttpServer<QTcpServer> mServer;
};

QTEST_GUILESS_MAIN(QCoroSocketNotifierTest)

#include "qcorosocketnotifier.moc"