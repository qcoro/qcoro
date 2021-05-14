// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "testhttpserver.h"
#include "testobject.h"
#include "qcoro/network.h"

#include <QHostAddress>
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

private Q_SLOTS:
    void init() {
        mServer.start(QHostAddress::LocalHost);
    }

    void cleanup() {
        mServer.stop();
    }

    addTest(Triggers)
    addTest(DoesntBlockEventLoop)
    addTest(DoesntCoAwaitNullReply)
    addTest(DoesntCoAwaitFinishedReply)

private:
    TestHttpServer<QTcpServer> mServer;
};

QTEST_GUILESS_MAIN(QCoroNetworkReplyTest)

#include "qnetworkreply.moc"
