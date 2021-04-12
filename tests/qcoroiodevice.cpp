// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "testobject.h"
#include "testhttpserver.h"
#include "qcoro/coro.h"

#include <QTcpServer>
#include <QNetworkRequest>
#include <QNetworkAccessManager>

class QCoroIODeviceTest: public QCoro::TestObject<QCoroIODeviceTest>
{
    Q_OBJECT

private:
    QCoro::Task<> testReadAllTriggers_coro(QCoro::TestContext context) {
        QNetworkAccessManager nam;

        auto *reply = nam.get(QNetworkRequest{QStringLiteral("http://127.0.0.1:%1/stream").arg(mServer.port())});

        int trigger = 0;
        QByteArray line, data;
        do {
            line = co_await QCoro::coro(reply).readAll();
            data += line;
            ++trigger;
        } while (data.size() < 70);

        QCORO_COMPARE(data.size(), reply->rawHeader("Content-Length").toInt());
        QCORO_VERIFY(trigger > 9);
    }

    QCoro::Task<> testReadTriggers_coro(QCoro::TestContext context) {
        QNetworkAccessManager nam;

        auto *reply = nam.get(QNetworkRequest{QStringLiteral("http://127.0.0.1:%1/stream").arg(mServer.port())});

        QByteArray data;
        int triggers = 0;
        while (!reply->isFinished()) {
            data += co_await QCoro::coro(reply).read(1);
            ++triggers;
        }

        QCORO_COMPARE(triggers, data.size());
    }

    QCoro::Task<> testReadLineTriggers_coro(QCoro::TestContext context) {
        QNetworkAccessManager nam;

        auto *reply = nam.get(QNetworkRequest{QStringLiteral("http://127.0.0.1:%1/stream").arg(mServer.port())});

        QByteArrayList lines;
        while (!reply->isFinished()) {
            lines.push_back(co_await QCoro::coro(reply).readLine());
        }

        QCORO_COMPARE(lines.size(), 10);
    }

private Q_SLOTS:
    void init() {
        mServer.start(QHostAddress::LocalHost);
    }

    void cleanup() {
        mServer.stop();
    }


    addTest(ReadAllTriggers)
    addTest(ReadTriggers)
    addTest(ReadLineTriggers)

private:
    TestHttpServer<QTcpServer> mServer;
};

QTEST_GUILESS_MAIN(QCoroIODeviceTest)

#include "qcoroiodevice.moc"
