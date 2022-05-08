// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "testhttpserver.h"
#include "testobject.h"
#include "qcoroiodevice_macros.h"

#include "qcoro/network/qcoronetworkreply.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QTcpServer>

class QCoroNetworkReplyTest : public QCoro::TestObject<QCoroNetworkReplyTest> {
    Q_OBJECT

private:
    QCoro::Task<> testTriggers_coro(QCoro::TestContext) {
        QNetworkAccessManager nam;
        auto reply = std::unique_ptr<QNetworkReply>(nam.get(buildRequest()));

        co_await reply.get();

        QCORO_VERIFY(reply->isFinished());
        QCORO_COMPARE(reply->error(), QNetworkReply::NoError);
        QCORO_COMPARE(reply->readAll(), "abcdef");
    }

    QCoro::Task<> testQCoroWrapperTriggers_coro(QCoro::TestContext) {
        QNetworkAccessManager nam;
        auto reply = std::unique_ptr<QNetworkReply>(nam.get(buildRequest()));

        co_await qCoro(reply.get()).waitForFinished();

        QCORO_VERIFY(reply->isFinished());
        QCORO_COMPARE(reply->error(), QNetworkReply::NoError);
        QCORO_COMPARE(reply->readAll(), "abcdef");
    }

    void testThenQCoroWrapperTriggers_coro(TestLoop &el) {
        QNetworkAccessManager nam;
        auto reply = std::unique_ptr<QNetworkReply>(nam.get(buildRequest()));

        bool called = false;
        qCoro(reply.get()).waitForFinished().then([&](bool finished) {
            called = true;
            el.quit();
            QVERIFY(finished);
        });
        el.exec();
        QVERIFY(reply->isFinished());
        QCOMPARE(reply->error(), QNetworkReply::NoError);
        QCOMPARE(reply->readAll(), "abcdef");
        QVERIFY(called);
    }

    QCoro::Task<> testDoesntBlockEventLoop_coro(QCoro::TestContext) {
        QCoro::EventLoopChecker eventLoopResponsive;
        QNetworkAccessManager nam;

        auto reply = std::unique_ptr<QNetworkReply>(
            nam.get(buildRequest(QStringLiteral("block"))));

        co_await reply.get();

        QCORO_VERIFY(eventLoopResponsive);
        QCORO_VERIFY(reply->isFinished());
        QCORO_COMPARE(reply->error(), QNetworkReply::NoError);
        QCORO_COMPARE(reply->readAll(), "abcdef");
    }

    QCoro::Task<> testDoesntCoAwaitNullReply_coro(QCoro::TestContext test) {
        test.setShouldNotSuspend();
        mServer.setExpectTimeout(true);

        QNetworkReply *reply = nullptr;

        co_await reply;

        delete reply;
    }

    QCoro::Task<> testDoesntCoAwaitFinishedReply_coro(QCoro::TestContext test) {
        QNetworkAccessManager nam;
        auto reply = std::unique_ptr<QNetworkReply>(nam.get(buildRequest()));

        co_await reply.get();

        QCORO_VERIFY(reply->isFinished());

        test.setShouldNotSuspend();
        co_await reply.get();
    }

    QCoro::Task<> testReadAllTriggers_coro(QCoro::TestContext) {
        QNetworkAccessManager nam;

        auto reply = std::unique_ptr<QNetworkReply>(
            nam.get(buildRequest(QStringLiteral("stream"))));

        QCORO_TEST_IODEVICE_READALL(*reply);

        QCORO_COMPARE(data.size(), reply->rawHeader("Content-Length").toInt());
    }

    void testThenReadAllTriggers_coro(TestLoop &el) {
        QNetworkAccessManager nam;
        auto reply = std::unique_ptr<QNetworkReply>(
            nam.get(buildRequest(QStringLiteral("block"))));

        bool called = false;
        qCoro(reply.get()).readAll().then([&](const QByteArray &data) {
            called = true;
            el.quit();
            QVERIFY(!data.isEmpty());
        });
        el.exec();
        QVERIFY(called);
    }

    QCoro::Task<> testReadTriggers_coro(QCoro::TestContext) {
        QNetworkAccessManager nam;

        auto reply = std::unique_ptr<QNetworkReply>(
            nam.get(buildRequest(QStringLiteral("stream"))));

        QCORO_TEST_IODEVICE_READ(*reply);

        QCORO_COMPARE(data.size(), reply->rawHeader("Content-Length").toInt());
    }

    void testThenReadTriggers_coro(TestLoop &el) {
        QNetworkAccessManager nam;
        auto reply = std::unique_ptr<QNetworkReply>(
            nam.get(buildRequest(QStringLiteral("block"))));
        bool called = false;
        qCoro(reply.get()).read(1).then([&](const QByteArray &data) {
            called = true;
            el.quit();
            QCOMPARE(data.size(), 1);
        });
        el.exec();
        QVERIFY(called);
    }

    QCoro::Task<> testReadLineTriggers_coro(QCoro::TestContext) {
        QNetworkAccessManager nam;
        auto reply = std::unique_ptr<QNetworkReply>(
            nam.get(buildRequest(QStringLiteral("stream"))));

        QCORO_TEST_IODEVICE_READLINE(*reply);
        QCORO_COMPARE(lines.size(), 10);
    }

    void testThenReadLineTriggers_coro(TestLoop &el) {
        QNetworkAccessManager nam;
        auto reply = std::unique_ptr<QNetworkReply>(
            nam.get(buildRequest(QStringLiteral("block"))));
        bool called = false;
        qCoro(reply.get()).readLine().then([&](const QByteArray &data) {
            called = true;
            el.quit();
            QVERIFY(!data.isEmpty());
        });
        el.exec();
        QVERIFY(called);
    }

private Q_SLOTS:
    void init() {
        mServer.start(QHostAddress::LocalHost);
    }

    void cleanup() {
        mServer.stop();
    }

    addTest(Triggers)
    addCoroAndThenTests(QCoroWrapperTriggers)
    addTest(DoesntBlockEventLoop)
    addTest(DoesntCoAwaitNullReply)
    addTest(DoesntCoAwaitFinishedReply)
    addCoroAndThenTests(ReadAllTriggers)
    addCoroAndThenTests(ReadTriggers)
    addCoroAndThenTests(ReadLineTriggers)

private:
    QNetworkRequest buildRequest(const QString &path = QString()) {
        return QNetworkRequest{
            QUrl{QStringLiteral("http://127.0.0.1:%1/%2").arg(mServer.port()).arg(path)}
        };
    }

    TestHttpServer<QTcpServer> mServer;
};

QTEST_GUILESS_MAIN(QCoroNetworkReplyTest)

#include "qcoronetworkreply.moc"
