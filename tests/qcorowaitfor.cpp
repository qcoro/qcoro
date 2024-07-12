// SPDX-FileCopyrightText: 2024 Joey Richey <joey@dogsplayingpoker.org>
//
// SPDX-License-Identifier: MIT

#include "testobject.h"

#include "qcorotimer.h"

#include <chrono>

class QCoroWaitForTest : public QCoro::TestObject<QCoroWaitForTest> {
    Q_OBJECT

private:
    QCoro::Task<void> testPrimitiveType_coro(QCoro::TestContext ctx)
    {
        ctx.setShouldNotSuspend();
        auto task_test = []() -> QCoro::Task<int> {
            co_return 7;
        };

        const int ret = QCoro::waitFor(task_test());
        QCORO_VERIFY(ret == 7);
    }

    QCoro::Task<void> testDefaultConstructible_coro(QCoro::TestContext ctx)
    {
        ctx.setShouldNotSuspend();
        auto task_test = []() -> QCoro::Task<std::string> {
            co_return "seven";
        };

        const std::string ret = QCoro::waitFor(task_test());
        QCORO_VERIFY(ret == "seven");
    }

    QCoro::Task<void> testNonDefaultConstructible_coro(QCoro::TestContext ctx)
    {
        ctx.setShouldNotSuspend();

        struct test_struct {
            explicit test_struct(int i_) : i(i_) {}
            int i;
        };

        auto task_test = []() -> QCoro::Task<test_struct> {
            co_return test_struct(7);
        };

        const test_struct ret = QCoro::waitFor(task_test());
        QCORO_VERIFY(ret.i == 7);
    }

private Q_SLOTS:
    addTest(PrimitiveType)
    addTest(DefaultConstructible)
    addTest(NonDefaultConstructible)
};

QTEST_GUILESS_MAIN(QCoroWaitForTest)

#include "qcorowaitfor.moc"
