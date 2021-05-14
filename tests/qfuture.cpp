// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "testobject.h"
#include "qcoro/future.h"

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

    QCoro::Task<> testReturnsResult_coro(QCoro::TestContext) {
        const QString result = co_await QtConcurrent::run([] {
            std::this_thread::sleep_for(100ms);
            return QStringLiteral("42");
        });

        QCORO_COMPARE(result, QStringLiteral("42"));
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

    QCoro::Task<> testDoesntCoAwaitCanceledFuture_coro(QCoro::TestContext test) {
        test.setShouldNotSuspend();

        QFuture<void> future;
        co_await future;
    }

private Q_SLOTS:
    addTest(Triggers)
    addTest(ReturnsResult)
    addTest(DoesntBlockEventLoop)
    addTest(DoesntCoAwaitFinishedFuture)
    addTest(DoesntCoAwaitCanceledFuture)
};

QTEST_GUILESS_MAIN(QCoroFutureTest)

#include "qfuture.moc"
