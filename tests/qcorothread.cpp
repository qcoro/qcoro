// SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "testobject.h"

#include "qcoro/core/qcorothread.h"
#include "qcoro/core/qcorosignal.h"

#include <QThread>
#include <QScopeGuard>

#include <thread>
#include <memory>

using namespace std::chrono_literals;

class QCoroThreadTest : public QCoro::TestObject<QCoroThreadTest> {
    Q_OBJECT

private:
    QCoro::Task<> testWaitForStarted_coro(QCoro::TestContext) {
        std::unique_ptr<QThread> thread(QThread::create([]() {
            std::this_thread::sleep_for(100ms);
        }));

        const auto threadGuard = qScopeGuard([&]() { thread->wait(); });

        QCORO_DELAY(thread->start());

        const bool ok = co_await qCoro(thread.get()).waitForStarted();
        QCORO_VERIFY(thread->isRunning());
        QCORO_VERIFY(ok);
    }

    QCoro::Task<> testWaitForFinished_coro(QCoro::TestContext) {
        std::unique_ptr<QThread> thread(QThread::create([]() {
            std::this_thread::sleep_for(100ms);
        }));

        thread->start();
        co_await qCoro(thread.get()).waitForStarted();
        QCORO_VERIFY(thread->isRunning());
        const bool ok = co_await qCoro(thread.get()).waitForFinished();
        QCORO_VERIFY(thread->isFinished());

        QCORO_VERIFY(ok);
    }

    QCoro::Task<> testMoveToThread_coro(QCoro::TestContext) {
        QThread newThread;
        newThread.start();

        QCORO_COMPARE(QThread::currentThread(), QCoreApplication::instance()->thread());

        co_await QCoro::moveToThread(&newThread);

        QCORO_COMPARE(QThread::currentThread(), &newThread);

        co_await QCoro::moveToThread(qApp->thread());

        QCORO_COMPARE(QThread::currentThread(), QCoreApplication::instance()->thread());

        newThread.exit();
        newThread.wait();
    }

private Q_SLOTS:
    addTest(WaitForStarted)
    addTest(WaitForFinished)
    addTest(MoveToThread)
};


QTEST_GUILESS_MAIN(QCoroThreadTest)

#include "qcorothread.moc"
