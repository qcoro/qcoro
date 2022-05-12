// SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "asyncgenerator.h"
#include "testobject.h"
#include "qcorotimer.h"

#include <vector>

#include <QScopeGuard>

class AsyncGeneratorTest : public QCoro::TestObject<AsyncGeneratorTest> {
    Q_OBJECT
private:
    QCoro::Task<> testGenerator_coro(QCoro::TestContext) {
        const auto createGenerator = []() -> QCoro::AsyncGenerator<int> {
            for (int i = 0; i < 10; i++) {
                QTimer timer;
                timer.start(50ms);
                co_await qCoro(timer).waitForTimeout();
                co_yield i;
            }
        };

        std::vector<int> values;
        QCORO_FOREACH(int val, createGenerator()) {
            values.push_back(val);
        }

        QCORO_COMPARE(values, (std::vector<int>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9}));
    }

    QCoro::Task<> testSyncGenerator_coro(QCoro::TestContext ctx) {
        ctx.setShouldNotSuspend();

        const auto createGenerator = []() -> QCoro::AsyncGenerator<int> {
            for (int i = 0; i < 10; ++i) {
                co_yield i;
            }
        };

        std::vector<int> values;
        QCORO_FOREACH(int val, createGenerator()) {
            values.push_back(val);
        }

        QCORO_COMPARE(values, (std::vector<int>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9}));
    }

    QCoro::Task<> testTerminateSuspendedGenerator_coro(QCoro::TestContext ctx) {
        ctx.setShouldNotSuspend();

        bool destroyed = false;
        const auto createGenerator = [&destroyed]() -> QCoro::AsyncGenerator<int> {
            const auto guard = qScopeGuard([&destroyed]() {
                destroyed = true;
            });

            const auto pointer = std::make_unique<QString>(
                QStringLiteral("This should get destroyed. If not, ASAN will catch it."));

            while (true) {
                co_yield 42;
            }
        };

        {
            auto generator = createGenerator();
            const auto it = co_await generator.begin();
            QCORO_COMPARE(*it, 42);
        } // The generator gets destroyed here. Everything on generator's stack is destroyed.

        QCORO_VERIFY(destroyed);
    }

    QCoro::Task<> testEmptyGenerator_coro(QCoro::TestContext ctx) {
        ctx.setShouldNotSuspend();

        const auto createGenerator = []() -> QCoro::AsyncGenerator<int> {
            if (false) {
                co_yield 42;
            }
        };

        auto generator = createGenerator();
        QCORO_COMPARE(co_await generator.begin(), generator.end());
    }

private Q_SLOTS:
    addTest(Generator)
    addTest(SyncGenerator)
    addTest(TerminateSuspendedGenerator)
    addTest(EmptyGenerator)
};

QTEST_GUILESS_MAIN(AsyncGeneratorTest)

#include "asyncgenerator.moc"
