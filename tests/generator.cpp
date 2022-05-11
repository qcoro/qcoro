// SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "generator.h"

#include <QObject>
#include <QTest>

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
        while (generator) {
            values.emplace_back(*generator);
        }

        QCOMPARE(values.size(), 10);
        QCOMPARE(values, (std::vector<int>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9}));
    }
};

QTEST_GUILESS_MAIN(GeneratorTest)

#include "generator.moc"
