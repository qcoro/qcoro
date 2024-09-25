// SPDX-FileCopyrightText: 2024 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "qcorotest.h"
#include "testobject.h"
#include "qcorotimer.h"
#include <qobject.h>
#include "qcorolazytask.h"

using namespace std::chrono_literals;

class QCoroLazyTaskTest : public QCoro::TestObject<QCoroLazyTaskTest>
{
    Q_OBJECT

private:
    QCoro::Task<> testSyncLazyCoroutineStarts_coro(QCoro::TestContext context) {
        context.setShouldNotSuspend();

        constexpr auto coro = [](bool &started) -> QCoro::LazyTask<> {
            started = true;
            co_return;
        };

        bool started = false;
        const auto task = coro(started);
        QCORO_VERIFY(!started);

        co_await task;

        QCORO_VERIFY(started);
    }

    QCoro::Task<> testLazyCoroutineStarts_coro(QCoro::TestContext) {
        constexpr auto coro = [](bool &started, bool &resumed) -> QCoro::LazyTask<> {
            started = true;
            co_await QCoro::sleepFor(1ms);
            resumed = true;
        };

        bool started = false;
        bool resumed = false;
        const auto task = coro(started, resumed);
        QCORO_VERIFY(!started);

        co_await task;

        QCORO_VERIFY(started);
        QCORO_VERIFY(resumed);
    }

    QCoro::Task<> testNonVoidSyncLazyCoroutineStarts_coro(QCoro::TestContext context) {
        context.setShouldNotSuspend();

        constexpr auto coro = [](bool &started) -> QCoro::LazyTask<int> {
            started = true;
            co_return 42;
        };

        bool started = false;
        const auto task = coro(started);
        QCORO_VERIFY(!started);

        const auto result = co_await task;

        QCORO_VERIFY(started);
        QCORO_COMPARE(result, 42);
    }

    QCoro::Task<> testNonVoidLazyCoroutineStarts_coro(QCoro::TestContext) {
        constexpr auto coro = [](bool &started, bool &resumed) -> QCoro::LazyTask<int> {
            started = true;
            co_await QCoro::sleepFor(1ms);
            resumed = true;
            co_return 42;
        };

        bool started = false;
        bool resumed = false;
        const auto task = coro(started, resumed);
        QCORO_VERIFY(!started);

        const auto result = co_await task;

        QCORO_VERIFY(started);
        QCORO_VERIFY(resumed);
        QCORO_COMPARE(result, 42);
    }

    QCoro::Task<> testEagerInsideLazy_coro(QCoro::TestContext) {
        constexpr auto coro = []() -> QCoro::LazyTask<int> {
            co_return co_await []() -> QCoro::Task<int> {
                co_await QCoro::sleepFor(1ms);
                co_return 42;
            }();
        };

        const auto task = coro();

        const auto result = co_await task;

        QCORO_COMPARE(result, 42);
    }

    QCoro::Task<> testThenLazyContinuation_coro(QCoro::TestContext) {
        constexpr auto coro = []() -> QCoro::LazyTask<int> {
            co_await QCoro::sleepFor(1ms);
            co_return 42;
        };

        const auto task = coro().then([](int result) -> QCoro::LazyTask<QString> {
            co_await QCoro::sleepFor(1ms);
            co_return QString::number(result);
        });

        const auto result = co_await task;
        QCORO_COMPARE(result, QStringLiteral("42"));
    }

    QCoro::Task<> testThenEagerContinuation_coro(QCoro::TestContext) {
        constexpr auto coro = []() -> QCoro::LazyTask<int> {
            co_await QCoro::sleepFor(1ms);
            co_return 42;
        };

        const auto task = coro().then([](int result) -> QCoro::Task<int> {
            co_await QCoro::sleepFor(1ms);
            co_return result;
        });

        const auto result = co_await task;
        QCORO_COMPARE(result, 42);
    }

    QCoro::Task<> testThenNonCoroutineContinuation_coro(QCoro::TestContext) {
        constexpr auto coro = []() -> QCoro::LazyTask<int> {
            co_await QCoro::sleepFor(1ms);
            co_return 42;
        };

        const auto task = coro().then([](int result) {
            return QString::number(result); 
        });
        static_assert(std::is_same_v<decltype(task), const QCoro::LazyTask<QString>>);

        const auto result = co_await task;
        QCORO_COMPARE(result, QStringLiteral("42"));
    }

private Q_SLOTS:
    addTest(SyncLazyCoroutineStarts)
    addTest(LazyCoroutineStarts)
    addTest(NonVoidSyncLazyCoroutineStarts)
    addTest(NonVoidLazyCoroutineStarts)
    addTest(EagerInsideLazy)
    addTest(ThenLazyContinuation)
    addTest(ThenEagerContinuation)
    addTest(ThenNonCoroutineContinuation)

    void testWaitFor() {
        auto coro = []() -> QCoro::LazyTask<int> {
            co_await QCoro::sleepFor(1ms);
            co_return 42;
        };

        const auto result = QCoro::waitFor(coro());
        QCOMPARE(result, 42);
    }
};


QTEST_GUILESS_MAIN(QCoroLazyTaskTest)

#include "qcorolazytask.moc"
