// SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "qcoroasyncgenerator.h"
#include "qcorotimer.h"
#include "testobject.h"

#include <vector>

#include <QScopeGuard>

struct Nocopymove {
    explicit constexpr Nocopymove(int val): val(val) {}
    Nocopymove(const Nocopymove &) = delete;
    Nocopymove &operator=(const Nocopymove &) = delete;
    Nocopymove(Nocopymove &&) = delete;
    Nocopymove &operator=(Nocopymove &&) = delete;
    ~Nocopymove() = default;

    int val;
};

struct Moveonly {
    explicit constexpr Moveonly(int val): val(val) {}
    Moveonly(const Moveonly &) = delete;
    Moveonly &operator=(const Moveonly &) = delete;
    Moveonly(Moveonly &&) noexcept = default;
    Moveonly &operator=(Moveonly &&) noexcept = default;
    ~Moveonly() = default;

    int val;
};

QCoro::Task<> sleep(std::chrono::milliseconds ms) {
    QTimer timer;
    timer.start(ms);
    co_await timer;
}

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
            if (false) { // NOLINT(readability-simplify-boolean-expr)
                co_yield 42;
            }
        };

        auto generator = createGenerator();
        QCORO_COMPARE(co_await generator.begin(), generator.end());
    }

    QCoro::Task<> testReferenceGenerator_coro(QCoro::TestContext) {
        const auto createGenerator = []() -> QCoro::AsyncGenerator<Nocopymove &> {
            for (int i = 0; i < 8; i += 2) {
                Nocopymove val{i};
                co_await sleep(10ms);
                co_yield val;
                QCORO_COMPARE(val.val, i + 1);
            }
        };

        int testvalue = 0;
        QCORO_FOREACH(Nocopymove &val, createGenerator()) {
            QCORO_COMPARE(val.val, testvalue);
            ++val.val;
            testvalue += 2;
        }
        QCORO_COMPARE(testvalue, 8);
    }

    QCoro::Task<> testConstReferenceGenerator_coro(QCoro::TestContext) {
        const auto createGenerator = []() -> QCoro::AsyncGenerator<const Nocopymove &> {
            for (int i = 0; i < 4; ++i) {
                const Nocopymove value{i};
                co_await sleep(10ms);
                co_yield value;
            }
        };

        int testvalue = 0;
        QCORO_FOREACH(const Nocopymove &val, createGenerator()) {
            QCORO_COMPARE(val.val, testvalue++);
        }
        QCORO_COMPARE(testvalue, 4);
    }

    QCoro::Task<> testMoveonlyGenerator_coro(QCoro::TestContext) {
        const auto createGenerator = []() -> QCoro::AsyncGenerator<Moveonly> {
            for (int i = 0; i < 4; ++i) {
                Moveonly value{i};
                co_await sleep(10ms);
                co_yield std::move(value);
            }
        };

        auto generator = createGenerator();
        int testvalue = 0;
        for (auto it = co_await generator.begin(), end = generator.end(); it != end; co_await ++it) {
            Moveonly value = std::move(*it);
            QCORO_COMPARE(value.val, testvalue++);
        }
        QCORO_COMPARE(testvalue, 4);
    }

private Q_SLOTS:
    addTest(Generator)
    addTest(SyncGenerator)
    addTest(TerminateSuspendedGenerator)
    addTest(EmptyGenerator)
    addTest(ReferenceGenerator)
    addTest(ConstReferenceGenerator)
    addTest(MoveonlyGenerator)
};

QTEST_GUILESS_MAIN(AsyncGeneratorTest)

#include "qcoroasyncgenerator.moc"
