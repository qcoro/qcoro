// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QEventLoop>
#include <QObject>
#include <QTest>
#include <QTimer>
#include <QVariant>

#include "qcorotask.h"
#include "qcorotest.h"
#include "testmacros.h"
#include "testloop.h"

#include <chrono>

using namespace std::chrono_literals;

namespace QCoro {

class TestContext {
public:
    TestContext(QEventLoop &el);
    TestContext(TestContext &&) noexcept;
    TestContext(const TestContext &) = delete;

    ~TestContext();

    TestContext &operator=(TestContext &&) noexcept;
    TestContext &operator=(const TestContext &) = delete;

    void setShouldNotSuspend();

private:
    QEventLoop *mEventLoop = {};
};

class EventLoopChecker : public QTimer {
    Q_OBJECT
public:
    explicit EventLoopChecker(int minTicks = 10, std::chrono::milliseconds interval = 5ms)
        : mMinTicks{minTicks} {
        connect(this, &EventLoopChecker::timeout, this, [this]() { ++mTick; });
        setInterval(interval);
        start();
    }

    operator bool() const {
        if (mTick < mMinTicks) {
            qDebug() << "EventLoopChecker failed: ticks=" << mTick << ", minTicks=" << mMinTicks;
        }
        return mTick >= mMinTicks;
    }

private:
    int mTick = 0;
    int mMinTicks = 10;
};

template<typename TestClass>
class TestObject : public QObject {
protected:
    explicit TestObject(QObject *parent = nullptr)
        : QObject(parent)
    {}

    void coroWrapper(QCoro::Task<> (TestClass::*testFunction)(TestContext)) {
        QEventLoop el;
        QTimer::singleShot(5s, &el, [&el]() mutable { el.exit(1); });

        (static_cast<TestClass *>(this)->*testFunction)(el);

        bool testFinished = el.property("testFinished").toBool();
        const bool shouldNotSuspend = el.property("shouldNotSuspend").toBool();
        if (testFinished) {
            QVERIFY(shouldNotSuspend);
        } else {
            QVERIFY(!shouldNotSuspend);

            const auto result = el.exec();
            QVERIFY2(result == 0, "Test function has timed out");

            testFinished = el.property("testFinished").toBool();
            QVERIFY(testFinished);
        }
    }
};

#define addTest(name)                                                                              \
    void test##name() {                                                                            \
        coroWrapper(&std::remove_cvref_t<decltype(*this)>::test##name##_coro);                     \
    }


#define addThenTest(name)                                                                          \
    void testThen##name() {                                                                        \
        TestLoop loop;                                                                             \
        testThen##name##_coro(loop);                                                               \
    }

#define addCoroAndThenTests(name) \
    addTest(name) \
    addThenTest(name)

} // namespace QCoro
