// SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "asyncgenerator.h"
#include "testobject.h"
#include "qcorotimer.h"

#include <vector>

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

private Q_SLOTS:
    addTest(Generator)
};

QTEST_GUILESS_MAIN(AsyncGeneratorTest)

#include "asyncgenerator.moc"
