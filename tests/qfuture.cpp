// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "testobject.h"

#include "qcorofuture.h"

#include <QtConcurrentRun>

#include <thread>

class QCoroFutureTest : public QCoro::TestObject<QCoroFutureTest> {
    Q_OBJECT
private:
    QCoro::Task<> testTriggers_coro(QCoro::TestContext) {
        auto future = QtConcurrent::run([] { std::this_thread::sleep_for(100ms); });
        co_await future;

        QCORO_VERIFY(future.isFinished());
    }

    QCoro::Task<> testQCoroWrapperTriggers_coro(QCoro::TestContext) {
        auto future = QtConcurrent::run([] { std::this_thread::sleep_for(100ms); });
        co_await qCoro(future).waitForFinished();

        QCORO_VERIFY(future.isFinished());
    }

    void testThenQCoroWrapperTriggers_coro(TestLoop &el) {
        auto future = QtConcurrent::run([] { std::this_thread::sleep_for(100ms); });

        bool called = false;
        qCoro(future).waitForFinished().then([&]() {
            called = true;
            el.quit();
        });
        el.exec();
        QVERIFY(called);
    }

    QCoro::Task<> testReturnsResult_coro(QCoro::TestContext) {
        const QString result = co_await QtConcurrent::run([] {
            std::this_thread::sleep_for(100ms);
            return QStringLiteral("42");
        });

        QCORO_COMPARE(result, QStringLiteral("42"));
    }

    void testThenReturnsResult_coro(TestLoop &el) {
        const auto future = QtConcurrent::run([] {
            std::this_thread::sleep_for(100ms);
            return QStringLiteral("42");
        });

        bool called = false;
        qCoro(future).waitForFinished().then([&](const QString &result) {
            called = true;
            el.quit();
            QCOMPARE(result, QStringLiteral("42"));
        });
        el.exec();
        QVERIFY(called);
    }

    QCoro::Task<> testDoesntBlockEventLoop_coro(QCoro::TestContext) {
        QCoro::EventLoopChecker eventLoopResponsive;

        co_await QtConcurrent::run([] { std::this_thread::sleep_for(500ms); });

        QCORO_VERIFY(eventLoopResponsive);
    }

    QCoro::Task<> testDoesntCoAwaitFinishedFuture_coro(QCoro::TestContext test) {
        auto future = QtConcurrent::run([] { std::this_thread::sleep_for(100ms); });
        co_await future;

        QCORO_VERIFY(future.isFinished());

        test.setShouldNotSuspend();
        co_await future;
    }

    void testThenDoesntCoAwaitFinishedFuture_coro(TestLoop &el) {
        auto future = QtConcurrent::run([] { std::this_thread::sleep_for(1ms); });
        QTest::qWait((100ms).count());
        QVERIFY(future.isFinished());

        bool called = false;
        qCoro(future).waitForFinished().then([&]() {
            called = true;
            el.quit();
        });
        el.exec();
        QVERIFY(called);
    }

    QCoro::Task<> testDoesntCoAwaitCanceledFuture_coro(QCoro::TestContext test) {
        test.setShouldNotSuspend();

        QFuture<void> future;
        co_await future;
    }

    void testThenDoesntCoAwaitCanceledFuture_coro(TestLoop &el) {
        QFuture<void> future;
        bool called = false;
        qCoro(future).waitForFinished().then([&]() {
            called = true;
            el.quit();
        });
        el.exec();
        QVERIFY(called);
    }

private Q_SLOTS:
    addTest(Triggers)
    addCoroAndThenTests(ReturnsResult)
    addTest(DoesntBlockEventLoop)
    addCoroAndThenTests(DoesntCoAwaitFinishedFuture)
    addCoroAndThenTests(DoesntCoAwaitCanceledFuture)
    addCoroAndThenTests(QCoroWrapperTriggers)
};

QTEST_GUILESS_MAIN(QCoroFutureTest)

#include "qfuture.moc"
