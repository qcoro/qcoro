// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "testobject.h"
#include "task.h"
#include "qcorotimer.h"

#include <QTest>
#include <QObject>
#include <QScopeGuard>

#include <chrono>

using namespace std::chrono_literals;

namespace {

QCoro::Task<> timer(std::chrono::milliseconds timeout) {
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
        co_await timer(100ms);
    }

    QCoro::Task<> testCoroutineValue_coro(QCoro::TestContext) {
        const auto coro = [](const QString &result) -> QCoro::Task<QString> {
            co_await timer(100ms);
            co_return result;
        };

        const auto value = QStringLiteral("Done!");
        const auto result = co_await coro(value);
        QCORO_COMPARE(result, value);
    }

    QCoro::Task<> testCoroutineMoveValue_coro(QCoro::TestContext) {
        const auto coro = [](const QString &result) -> QCoro::Task<std::unique_ptr<QString>> {
            co_await timer(100ms);
            co_return std::make_unique<QString>(result);
        };

        const auto value = QStringLiteral("Done ptr!");
        const auto result = co_await coro(value);
        QCORO_COMPARE(*result.get(), value);
    }

    QCoro::Task<> testSyncCoroutine_coro(QCoro::TestContext) {
        const auto coro = []() -> QCoro::Task<int> {
            co_return 42;
        };

        const auto result = co_await coro();
        QCORO_COMPARE(result, 42);
    }

    QCoro::Task<> testCoroutineWithException_coro(QCoro::TestContext) {
        const auto coro = []() -> QCoro::Task<int> {
            co_await timer(100ms);
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
            co_await timer(100ms);
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
            co_await timer(100ms);
            QCORO_VERIFY(!destroyed);
        };

        co_await coro();
        QCORO_VERIFY(destroyed);
    }

private Q_SLOTS:
    addTest(SimpleCoroutine)
    addTest(CoroutineValue)
    addTest(CoroutineMoveValue)
    // FIXME: Crashes with ASAN due to use-after-free
    //addTest(SyncCoroutine)
    addTest(CoroutineWithException)
    addTest(VoidCoroutineWithException)
    addTest(CoroutineFrameDestroyed)

    // TODO: Test timeout
    void testWaitFor() {
        QCoro::waitFor(timer(500ms));
    }

    // TODO: Test timeout
    void testWaitForWithValue() {
        const auto result = QCoro::waitFor([]() -> QCoro::Task<int> {
            co_await timer(100ms);
            co_return 42;
        }());
        QCOMPARE(result, 42);
    }

    void testIgnoredVoidTaskResult() {
        QEventLoop el;
        ignoreCoroutineResult(el, [&el]() -> QCoro::Task<> {
            co_await timer(300ms);
            el.quit();
        });
    }

    void testIgnoredValueTaskResult() {
        QEventLoop el;
        ignoreCoroutineResult(el, [&el]() -> QCoro::Task<QString> {
            co_await timer(300ms);
            el.quit();
            co_return QStringLiteral("Result");
        });
    }
};

QTEST_GUILESS_MAIN(QCoroTaskTest)

#include "task.moc"
