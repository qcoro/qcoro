// SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "generator.h"

#include <QObject>
#include <QTest>
#include <QScopeGuard>


// In clang <= 11 ASAN detects stack-use-after-scope in values.emplace_back(*it).
// I believe this is a bug in clang, since the codegen has change in clang 12 and
// the issue is no longer present. GCC and MSVC produce totally different assembly
// and the issue doesn't manifest there either.
// FIXME: Drop this once we bump minimum clang version to 12
// FIXME: Prove that this is really a bug in clang and try to workaround it.
#if defined(__clang__) && __clang_major__ <= 11
    #define CLANG11_DISABLE_ASAN __attribute__((no_sanitize("address")))
#else
    #define CLANG11_DISABLE_ASAN
#endif

class GeneratorTest : public QObject {
    Q_OBJECT
private Q_SLOTS:

    CLANG11_DISABLE_ASAN void testImmediateGenerator() {
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

        QCOMPARE(values.size(), 10);
        QCOMPARE(values, (std::vector<int>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9}));
    }

    CLANG11_DISABLE_ASAN void testTerminateSuspendedGenerator() {
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

    CLANG11_DISABLE_ASAN void testEmptyGenerator() {
        const auto createGenerator = []() -> QCoro::Generator<int> {
            if (false) {
                co_yield 42; // Make it a coroutine, except it never gets invoked.
            }
        };

        auto generator = createGenerator();
        const auto begin = generator.begin();
        QCOMPARE(begin, generator.end());
    }
};

QTEST_GUILESS_MAIN(GeneratorTest)

#include "generator.moc"
