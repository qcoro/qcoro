// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "testhttpserver.h"
#include "testobject.h"

#include "qcoro/network/qcoronetworkreply.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QTcpServer>

class QCoroNetworkReplyTest : public QCoro::TestObject<QCoroNetworkReplyTest> {
    Q_OBJECT

private:
    QCoro::Task<> testTriggers_coro(QCoro::TestContext) {
        QNetworkAccessManager nam;
        auto *reply =
            nam.get(QNetworkRequest{QStringLiteral("http://localhost:%1").arg(mServer.port())});

        co_await reply;

        QCORO_VERIFY(reply->isFinished());
        QCORO_COMPARE(reply->error(), QNetworkReply::NoError);
        QCORO_COMPARE(reply->readAll(), "abcdef");

        delete reply;
    }

    QCoro::Task<> testQCoroWrapperTriggers_coro(QCoro::TestContext) {
        QNetworkAccessManager nam;
        auto *reply =
            nam.get(QNetworkRequest{QStringLiteral("http://localhost:%1").arg(mServer.port())});

        co_await qCoro(reply).waitForFinished();

        QCORO_VERIFY(reply->isFinished());
        QCORO_COMPARE(reply->error(), QNetworkReply::NoError);
        QCORO_COMPARE(reply->readAll(), "abcdef");

        delete reply;
    }

    QCoro::Task<> testDoesntBlockEventLoop_coro(QCoro::TestContext) {
        QCoro::EventLoopChecker eventLoopResponsive;
        QNetworkAccessManager nam;

        auto *reply = nam.get(
            QNetworkRequest{QStringLiteral("http://localhost:%1/block").arg(mServer.port())});

        co_await reply;

        QCORO_VERIFY(eventLoopResponsive);
        QCORO_VERIFY(reply->isFinished());
        QCORO_COMPARE(reply->error(), QNetworkReply::NoError);
        QCORO_COMPARE(reply->readAll(), "abcdef");

        delete reply;
    }

    QCoro::Task<> testDoesntCoAwaitNullReply_coro(QCoro::TestContext test) {
        test.setShouldNotSuspend();

        QNetworkReply *reply = nullptr;

        co_await reply;

        delete reply;
    }

    QCoro::Task<> testDoesntCoAwaitFinishedReply_coro(QCoro::TestContext test) {
        QNetworkAccessManager nam;
        auto *reply =
            nam.get(QNetworkRequest{QStringLiteral("http://localhost:%1").arg(mServer.port())});

        co_await reply;

        QCORO_VERIFY(reply->isFinished());

        test.setShouldNotSuspend();
        co_await reply;

        delete reply;
    }


    QCoro::Task<> testReadAllTriggers_coro(QCoro::TestContext) {
        QNetworkAccessManager nam;

        auto *reply = nam.get(
            QNetworkRequest{QStringLiteral("http://127.0.0.1:%1/stream").arg(mServer.port())});

        QByteArray data;
        while (!reply->isFinished()) {
            data += co_await qCoro(reply).readAll();
        }
        data += reply->readAll(); // read what's left in the buffer

        QCORO_COMPARE(data.size(), reply->rawHeader("Content-Length").toInt());
    }

    QCoro::Task<> testReadTriggers_coro(QCoro::TestContext) {
        QNetworkAccessManager nam;

        auto *reply = nam.get(
            QNetworkRequest{QStringLiteral("http://127.0.0.1:%1/stream").arg(mServer.port())});

        QByteArray data;
        while (!reply->isFinished()) {
            data += co_await qCoro(reply).read(1);
        }
        data += reply->readAll(); // read what's left in the buffer

        QCORO_COMPARE(data.size(), reply->rawHeader("Content-Length").toInt());
    }

    QCoro::Task<> testReadLineTriggers_coro(QCoro::TestContext) {
        QNetworkAccessManager nam;

        auto *reply = nam.get(
            QNetworkRequest{QStringLiteral("http://127.0.0.1:%1/stream").arg(mServer.port())});

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

    addTest(Triggers)
    addTest(QCoroWrapperTriggers)
    addTest(DoesntBlockEventLoop)
    addTest(DoesntCoAwaitNullReply)
    addTest(DoesntCoAwaitFinishedReply)
    addTest(ReadAllTriggers)
    addTest(ReadTriggers)
    addTest(ReadLineTriggers)

private:
    TestHttpServer<QTcpServer> mServer;
};

QTEST_GUILESS_MAIN(QCoroNetworkReplyTest)

#include "qcoronetworkreply.moc"
