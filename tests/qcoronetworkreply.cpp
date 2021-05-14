// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "testobject.h"
#include "testhttpserver.h"
#include "qcoro/coro.h"

#include <QTcpServer>
#include <QNetworkRequest>
#include <QNetworkAccessManager>

class QCoroNetworkReplyTest: public QCoro::TestObject<QCoroNetworkReplyTest>
{
    Q_OBJECT

private:
    QCoro::Task<> testReadAllTriggers_coro(QCoro::TestContext) {
        QNetworkAccessManager nam;

        auto *reply = nam.get(QNetworkRequest{QStringLiteral("http://127.0.0.1:%1/stream").arg(mServer.port())});

        QByteArray data;
        while (!reply->isFinished()) {
            data += co_await qCoro(reply).readAll();
        }
        data += reply->readAll(); // read what's left in the buffer

        QCORO_COMPARE(data.size(), reply->rawHeader("Content-Length").toInt());
    }

    QCoro::Task<> testReadTriggers_coro(QCoro::TestContext) {
        QNetworkAccessManager nam;

        auto *reply = nam.get(QNetworkRequest{QStringLiteral("http://127.0.0.1:%1/stream").arg(mServer.port())});

        QByteArray data;
        while (!reply->isFinished()) {
            data += co_await qCoro(reply).read(1);
        }
        data += reply->readAll(); // read what's left in the buffer

        QCORO_COMPARE(data.size(), reply->rawHeader("Content-Length").toInt());
    }

    QCoro::Task<> testReadLineTriggers_coro(QCoro::TestContext) {
        QNetworkAccessManager nam;

        auto *reply = nam.get(QNetworkRequest{QStringLiteral("http://127.0.0.1:%1/stream").arg(mServer.port())});

        QByteArrayList lines;
        while (!reply->isFinished()) {
            const auto line = co_await qCoro(reply).readLine();
            if (!line.isNull()) {
                lines.push_back(line);
            }
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

QTEST_GUILESS_MAIN(QCoroNetworkReplyTest)

#include "qcoronetworkreply.moc"

