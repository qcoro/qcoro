#// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "testobject.h"
#include "qcoro/qcorocancellabletask.h"
#include "qcoro/core/qcorotimer.h"

#include <QScopeGuard>

class QCoroCancellableTaskTest : public QCoro::TestObject<QCoroCancellableTaskTest> {
    Q_OBJECT
private:
    QCoro::Task<> testCancellableTaskSyncResult_coro(QCoro::TestContext ctx) {
        ctx.setShouldNotSuspend();

        const auto result = co_await []() -> QCoro::CancellableTask<int> {
            co_return 42;
        }();

        QCORO_VERIFY(!result.isCancelled());
        QCORO_COMPARE(result.value(), 42);
    }

    QCoro::Task<> testCancellableTaskAsyncResult_coro(QCoro::TestContext) {
        const auto result = co_await []() -> QCoro::CancellableTask<int> {
            QTimer timer;
            timer.start(100ms);
            co_await timer;
            co_return 42;
        }();

        QCORO_VERIFY(!result.isCancelled());
        QCORO_COMPARE(result.value(), 42);
    }

    QCoro::Task<> testCancelCoroutineWithSimpleAwaitable_coro(QCoro::TestContext) {
        bool destroyed = false;
        bool notReached = true;
        {
            auto task = [&]() -> QCoro::CancellableTask<int> {
                const auto guard = qScopeGuard([&destroyed]() {
                    destroyed = true;
                });
                QTimer timer;
                timer.setSingleShot(true);
                timer.start(200ms);
                co_await timer;
                notReached = false; // We shouldn't get here
                co_return 42;
            }();
            QTimer::singleShot(0, this, [&task]() {
                task.cancel();
            });

            const auto result = co_await task;

            QCORO_VERIFY(result.isCancelled());
        } // only now the task's coroutine state is destroyed

        QCORO_VERIFY(destroyed);
        QCORO_VERIFY(notReached);
    }

    QCoro::Task<> testCancelCoroutineWithTask_coro(QCoro::TestContext) {
        bool task1Destroyed = false;
        bool task1NotReached = true;
        auto task1 = [&]() -> QCoro::CancellableTask<int> {
            const auto guard = qScopeGuard([&task1Destroyed]() {
                task1Destroyed = true;
            });
            QTimer timer;
            timer.setSingleShot(true);
            timer.start(200ms);
            co_await timer;
            task1NotReached = false;
            co_return 42;
        };
        bool task2Destroyed = false;
        bool task2NotReached = true;

        {
            auto task2 = [&]() -> QCoro::CancellableTask<int> {
                const auto guard = qScopeGuard([&task2Destroyed]() {
                    task2Destroyed = true;
                });
                const auto result = co_await task1();
                task2NotReached = false;
                co_return result;
            }();

            QTimer::singleShot(0, this, [&task2]() { task2.cancel(); });
            const auto result = co_await task2;

            QCORO_VERIFY(result.isCancelled());
        } // only now the task2 (and thus task1) are destroyed

        QCORO_VERIFY(task1Destroyed);
        QCORO_VERIFY(task1NotReached);
        QCORO_VERIFY(task2Destroyed);
        QCORO_VERIFY(task2NotReached);
    }

    QCoro::Task<> testCancelCoroutineWithThen_coro(QCoro::TestContext) {
        bool notCalled = true;

        struct NonDefaultConstructible {
            NonDefaultConstructible(const int value)
                : value(value)
            {}
            int value = 0;
        };

        auto task = []() -> QCoro::CancellableTask<NonDefaultConstructible> {
            QTimer timer;
            timer.start(100ms);
            co_await timer;
            co_return NonDefaultConstructible{42};
        }();

        QTimer::singleShot(0, this, [&task]() { task.cancel(); });

        const auto result = co_await task.then([&notCalled](NonDefaultConstructible result) {
            notCalled = false;
            return NonDefaultConstructible{result.value * 2};
        });

        QCORO_VERIFY(result.isCancelled());
        QCORO_VERIFY(notCalled);
    }

private Q_SLOTS:
    addTest(CancellableTaskSyncResult)
    addTest(CancellableTaskAsyncResult)
    addTest(CancelCoroutineWithSimpleAwaitable)
    addTest(CancelCoroutineWithTask)
    addTest(CancelCoroutineWithThen)

};

QTEST_GUILESS_MAIN(QCoroCancellableTaskTest)

#include "qcorocancellabletask.moc"
