// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "testobject.h"
#include "task.h"
#include "qcorotimer.h"

#include <QTest>
#include <QObject>
#include <QScopeGuard>
#include <QMetaObject>

#include <chrono>

using namespace std::chrono_literals;

namespace {

QCoro::Task<> timer(std::chrono::milliseconds timeout = 10ms) {
    QTimer timer;
    timer.setSingleShot(true);
    timer.start(timeout);
    co_await timer;
}

} // namespace

class QCoroTaskTest : public QCoro::TestObject<QCoroTaskTest>
{
    Q_OBJECT

private:
    template<typename Coro>
    void ignoreCoroutineResult(QEventLoop &el, Coro &&coro) {
        QTimer::singleShot(5s, &el, [&el]() mutable { el.exit(1); });

        coro();
        const int timeout = el.exec();
        QCOMPARE(timeout, 0);
    }

    QCoro::Task<> testSimpleCoroutine_coro(QCoro::TestContext) {
        co_await timer();
    }

    QCoro::Task<> testCoroutineValue_coro(QCoro::TestContext) {
        const auto coro = [](const QString &result) -> QCoro::Task<QString> {
            co_await timer();
            co_return result;
        };

        const auto value = QStringLiteral("Done!");
        const auto result = co_await coro(value);
        QCORO_COMPARE(result, value);
    }

    QCoro::Task<> testCoroutineMoveValue_coro(QCoro::TestContext) {
        const auto coro = [](const QString &result) -> QCoro::Task<std::unique_ptr<QString>> {
            co_await timer();
            co_return std::make_unique<QString>(result);
        };

        const auto value = QStringLiteral("Done ptr!");
        const auto result = co_await coro(value);
        QCORO_COMPARE(*result.get(), value);
    }

    QCoro::Task<> testSyncCoroutine_coro(QCoro::TestContext context) {
        context.setShouldNotSuspend();

        const auto coro = []() -> QCoro::Task<int> {
            co_return 42;
        };

        const auto result = co_await coro();
        QCORO_COMPARE(result, 42);
    }

    QCoro::Task<> testCoroutineWithException_coro(QCoro::TestContext) {
        const auto coro = []() -> QCoro::Task<int> {
            co_await timer();
            throw std::runtime_error("Invalid result");
            co_return 42;
        };

        try {
            const auto result = co_await coro();
            QCORO_FAIL("Exception was not propagated.");
            Q_UNUSED(result);
        } catch (const std::runtime_error &) {
            // OK
        } catch (...) {
            QCORO_FAIL("Exception type was not propagated, or other exception was thrown.");
        }
    }

    QCoro::Task<> testVoidCoroutineWithException_coro(QCoro::TestContext) {
        const auto coro = []() -> QCoro::Task<> {
            co_await timer();
            throw std::runtime_error("Error");
        };

        try {
            co_await coro();
            QCORO_FAIL("Exception was not propagated.");
        } catch (const std::runtime_error &) {
            // OK
        } catch (...) {
            QCORO_FAIL("Exception type was not propagated, or other exception was thrown.");
        }
    }

    QCoro::Task<> testCoroutineFrameDestroyed_coro(QCoro::TestContext) {
        bool destroyed = false;
        const auto coro = [&destroyed]() -> QCoro::Task<> {
            const auto guard = qScopeGuard([&destroyed]() mutable {
                destroyed = true;
            });

            QCORO_VERIFY(!destroyed);
            co_await timer();
            QCORO_VERIFY(!destroyed);
        };

        co_await coro();
        QCORO_VERIFY(destroyed);
    }

    QCoro::Task<> testExceptionPropagation_coro(QCoro::TestContext) {
        QCORO_VERIFY_EXCEPTION_THROWN(
            co_await []() -> QCoro::Task<int> {
                throw std::runtime_error("Test!");
                co_return 42;
            }(),
            std::runtime_error);

        QCORO_VERIFY_EXCEPTION_THROWN(
            co_await []() -> QCoro::Task<> {
                throw std::runtime_error("Test!");
            }(),
            std::runtime_error);

        QCORO_VERIFY_EXCEPTION_THROWN(
            co_await []() -> QCoro::Task<int> {
                co_await timer();
                throw std::runtime_error("Test!");
                co_return 42;
            }(),
            std::runtime_error);

        QCORO_VERIFY_EXCEPTION_THROWN(
            co_await []() -> QCoro::Task<> {
                co_await timer();
                throw std::runtime_error("Test!");
            }(),
            std::runtime_error);
    }

private Q_SLOTS:
    addTest(SimpleCoroutine)
    addTest(CoroutineValue)
    addTest(CoroutineMoveValue)
    addTest(SyncCoroutine)
    addTest(CoroutineWithException)
    addTest(VoidCoroutineWithException)
    addTest(CoroutineFrameDestroyed)
    addTest(ExceptionPropagation)

    // See https://github.com/danvratil/qcoro/issues/24
    void testEarlyReturn()
    {
        QEventLoop loop;

        const auto testReturn = [](bool immediate) -> QCoro::Task<bool> {
            if (immediate) {
                co_return true;
            } else {
                co_await timer();
                co_return true;
            }
        };

        bool immediateResult = false;
        bool delayedResult = false;

        const auto testImmediate = [&]() -> QCoro::Task<> {
            immediateResult = co_await testReturn(true);
        };

        const auto testDelayed = [&]() -> QCoro::Task<> {
            delayedResult = co_await testReturn(false);
            loop.quit();
        };

        QMetaObject::invokeMethod(
            &loop, [&]() { testImmediate(); }, Qt::QueuedConnection);
        QMetaObject::invokeMethod(
            &loop, [&]() { testDelayed(); }, Qt::QueuedConnection);

        loop.exec();

        QVERIFY(immediateResult);
        QVERIFY(delayedResult);
    }

    // TODO: Test timeout
    void testWaitFor() {
        QCoro::waitFor(timer());
    }

    // TODO: Test timeout
    void testWaitForWithValue() {
        const auto result = QCoro::waitFor([]() -> QCoro::Task<int> {
            co_await timer();
            co_return 42;
        }());
        QCOMPARE(result, 42);
    }

    void testIgnoredVoidTaskResult() {
        QEventLoop el;
        ignoreCoroutineResult(el, [&el]() -> QCoro::Task<> {
            co_await timer();
            el.quit();
        });
    }

    void testIgnoredValueTaskResult() {
        QEventLoop el;
        ignoreCoroutineResult(el, [&el]() -> QCoro::Task<QString> {
            co_await timer();
            el.quit();
            co_return QStringLiteral("Result");
        });
    }
};

QTEST_GUILESS_MAIN(QCoroTaskTest)

#include "task.moc"
