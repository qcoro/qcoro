// SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "qcorogenerator.h"
#include "qcorotest.h"

#include <QObject>
#include <QTest>
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

class GeneratorTest : public QObject {
    Q_OBJECT
private Q_SLOTS:

    void testImmediateGenerator() {
        const auto createGenerator = []() -> QCoro::Generator<int> {
            for (int value = 0; value < 10; ++value) {
                co_yield value;
            }
        };

        auto generator = createGenerator();
        std::vector<int> values;
        for (auto it = generator.begin(), end = generator.end(); it != end; ++it) {
            values.emplace_back(*it);
        }

        QCOMPARE(values.size(), 10U);
        QCOMPARE(values, (std::vector<int>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9}));
    }

    void testTerminateSuspendedGenerator() {
        bool destroyed = false;
        const auto createGenerator = [&destroyed]() -> QCoro::Generator<int> {
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
            const auto it = generator.begin();
            QCOMPARE(*it, 42);
        } // The generator gets destroyed here.

        QVERIFY(destroyed);
    }

    void testEmptyGenerator() {
        const auto createGenerator = []() -> QCoro::Generator<int> {
            if (false) { // NOLINT(readability-simplify-boolean-expr)
                co_yield 42; // Make it a coroutine, except it never gets invoked.
            }
        };

        auto generator = createGenerator();
        const auto begin = generator.begin();
        QCOMPARE(begin, generator.end());
    }

    void testConstReferenceGenerator() {
        const auto createGenerator = []() -> QCoro::Generator<const Nocopymove &> {
            for (int i = 0; i < 4; ++i) {
                const Nocopymove val(i);
                co_yield val;
            }
        };

        auto generator = createGenerator();
        auto it = generator.begin();
        int testval = 0;
        while (it != generator.end()) {
            const Nocopymove &value = *it;
            QCOMPARE(value.val, testval++);
            ++it;
        }
        QCOMPARE(testval, 4);
    }

    void testReferenceGenerator() {
        const auto createGenerator = []() -> QCoro::Generator<Nocopymove &> {
            for (int i = 0; i < 8; i += 2) {
                Nocopymove val(i);
                co_yield val;
                QCORO_COMPARE(val.val, i + 1);
            }
        };

        auto generator = createGenerator();
        auto it = generator.begin();
        int testval = 0;
        while (it != generator.end()) {
            Nocopymove &value = *it;
            QCOMPARE(value.val, testval);
            value.val += 1;
            testval += 2;
            ++it;
        }
    }

    void testMoveonlyGenerator() {
        const auto createGenerator = []() -> QCoro::Generator<Moveonly &&> {
            for (int i = 0; i < 4; ++i) {
                Moveonly value{i};
                co_yield std::move(value);
            }
        };

        auto generator = createGenerator();
        auto it = generator.begin();
        int testval = 0;
        while (it != generator.end()) {
            Moveonly val = std::move(*it);
            QCOMPARE(val.val, testval++);
            ++it;
        }
        QCOMPARE(testval, 4);
    }

    void testMovedGenerator() {
        const auto createGenerator = []() -> QCoro::Generator<int> {
            for (int i = 0; i < 4; ++i) {
                co_yield i;
            }
        };

        auto originalGenerator = createGenerator();
        auto generator = std::move(originalGenerator);
        auto it = generator.begin();
        int testval = 0;
        while (it != generator.end()) {
            int value = *it;
            QCOMPARE(value, testval++);
            ++it;
        }
        QCOMPARE(testval, 4);
    }

    void testException() {
        const auto createGenerator = []() -> QCoro::Generator<int> {
            for (int i = 0; i < 10; ++i) {
                if (i == 2) {
                    throw std::runtime_error("Two?! I can't handle two!!");
                }
                co_yield i;
            }
        };

        auto generator = createGenerator();
        auto it = generator.begin();
        QVERIFY(it != generator.end());
        QCOMPARE(*it, 0);
        ++it;
        QVERIFY(it != generator.end());
        QCOMPARE(*it, 1);

#if QT_VERSION >= QT_VERSION_CHECK(6, 3, 0)
        QVERIFY_THROWS_EXCEPTION(std::runtime_error, ++it);
#else
        QVERIFY_EXCEPTION_THROWN(++it, std::runtime_error);
#endif
        QCOMPARE(it, generator.end());
    }

    void testExceptionInBegin() {
        auto generator = []() -> QCoro::Generator<int> {
            throw std::runtime_error("Zero is too small!");
            co_yield 1;
        }();

#if QT_VERSION >= QT_VERSION_CHECK(6, 3, 0)
        QVERIFY_THROWS_EXCEPTION(std::runtime_error, generator.begin());
#else
        QVERIFY_EXCEPTION_THROWN(generator.begin(), std::runtime_error);
#endif
    }
};

QTEST_GUILESS_MAIN(GeneratorTest)

#include "qcorogenerator.moc"
