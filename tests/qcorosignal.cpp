// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "testobject.h"

#include "qcoro/core/qcorotimer.h"
#include "qcoro/core/qcorosignal.h"

#include <QTimer>

using namespace std::chrono_literals;

class SignalTest : public QObject {
    Q_OBJECT
public:
    explicit SignalTest() {
        QTimer::singleShot(100ms, this, [this]() {
            Q_EMIT voidSignal();
            Q_EMIT singleArg(QStringLiteral("YAY!"));
            Q_EMIT multiArg(QStringLiteral("YAY!"), 42, this);
            qDebug() << "Emission done";
        });
    }

Q_SIGNALS:
    void voidSignal();
    void singleArg(const QString &value);
    void multiArg(const QString &value, int number, QObject *ptr);
};

class QCoroSignalTest : public QCoro::TestObject<QCoroSignalTest> {
    Q_OBJECT

private:
    QCoro::Task<> testTriggers_coro(QCoro::TestContext) {
        SignalTest obj;

        co_await qCoro(&obj, &SignalTest::voidSignal);
        static_assert(
            std::is_same_v<decltype(qCoro(&obj, &SignalTest::voidSignal)), QCoro::Task<std::tuple<>>>);
    }

    QCoro::Task<> testReturnsValue_coro(QCoro::TestContext) {
        SignalTest obj;

        const auto result = co_await qCoro(&obj, &SignalTest::singleArg);
        static_assert(std::is_same_v<decltype(result), const QString>);
        QCORO_COMPARE(result, QStringLiteral("YAY!"));
    }

    QCoro::Task<> testReturnsTuple_coro(QCoro::TestContext) {
        SignalTest obj;

        const auto result = co_await qCoro(&obj, &SignalTest::multiArg);
        static_assert(std::is_same_v<decltype(result), const std::tuple<const QString &, int, QObject *>>);
        const auto [value, number, ptr] = result;
        QCORO_COMPARE(value, QStringLiteral("YAY!"));
        QCORO_COMPARE(number, 42);
        QCORO_COMPARE(ptr, &obj);
    }

    QCoro::Task<> testTimeoutTriggersVoid_coro(QCoro::TestContext) {
        SignalTest obj;

        const auto result = co_await qCoro(&obj, &SignalTest::voidSignal, 10ms);
        static_assert(std::is_same_v<decltype(result), const std::optional<std::tuple<>>>);
        QCORO_VERIFY(!result.has_value());
    }

    QCoro::Task<> testTimeoutVoid_coro(QCoro::TestContext) {
        SignalTest obj;

        const auto result = co_await qCoro(&obj, &SignalTest::voidSignal, 1s);
        static_assert(std::is_same_v<decltype(result), const std::optional<std::tuple<>>>);
        QCORO_VERIFY(result.has_value());
    }

    QCoro::Task<> testTimeoutTriggersValue_coro(QCoro::TestContext) {
        SignalTest obj;

        const auto result = co_await qCoro(&obj, &SignalTest::singleArg, 10ms);
        static_assert(std::is_same_v<decltype(result), const std::optional<QString>>);
        QCORO_VERIFY(!result.has_value());
    }

    QCoro::Task<> testTimeoutValue_coro(QCoro::TestContext) {
        SignalTest obj;

        const auto result = co_await qCoro(&obj, &SignalTest::singleArg, 1s);
        static_assert(std::is_same_v<decltype(result), const std::optional<QString>>);
        QCORO_VERIFY(result.has_value());
        QCORO_COMPARE(*result, QStringLiteral("YAY!"));
    }

    QCoro::Task<> testTimeoutTriggersTuple_coro(QCoro::TestContext) {
        SignalTest obj;

        const auto result = co_await qCoro(&obj, &SignalTest::multiArg, 10ms);
        static_assert(std::is_same_v<
                decltype(result),
                const std::optional<std::tuple<const QString &, int, QObject *>>>);
        QCORO_VERIFY(!result.has_value());
    }

    QCoro::Task<> testTimeoutTuple_coro(QCoro::TestContext) {
        SignalTest obj;

        const auto result = co_await qCoro(&obj, &SignalTest::multiArg, 1s);
        static_assert(std::is_same_v<
                decltype(result),
                const std::optional<std::tuple<const QString &, int, QObject *>>>);
        QCORO_VERIFY(result.has_value());
        QCORO_COMPARE(std::get<0>(*result), QStringLiteral("YAY!"));
        QCORO_COMPARE(std::get<1>(*result), 42);
        QCORO_COMPARE(std::get<2>(*result), &obj);
    }


    void testThenTriggers_coro(TestLoop &el) {
        SignalTest obj;
        bool triggered = false;

        qCoro(&obj, &SignalTest::voidSignal).then([&]() {
            triggered = true;
            el.quit();
        });
        el.exec();

        QVERIFY(triggered);
    }

    void testThenReturnsValue_coro(TestLoop &el) {
        SignalTest obj;
        std::optional<QString> value;

        qCoro(&obj, &SignalTest::singleArg).then([&](const QString &arg) {
            value = arg;
            el.quit();
        });
        el.exec();

        QVERIFY(value.has_value());
        QCOMPARE(*value, QStringLiteral("YAY!"));
    }

    void testThenReturnsTuple_coro(TestLoop &el) {
        SignalTest obj;
        std::optional<QString> str;
        std::optional<int> num;
        std::optional<QObject *> ptr;

        qCoro(&obj, &SignalTest::multiArg).then([&](const std::tuple<QString, int, QObject *> &args) {
            str = std::get<0>(args);
            num = std::get<1>(args);
            ptr = std::get<2>(args);
            el.quit();
        });
        el.exec();

        QVERIFY(str.has_value());
        QVERIFY(num.has_value());
        QVERIFY(ptr.has_value());
        QCOMPARE(*str, QStringLiteral("YAY!"));
        QCOMPARE(*num, 42);
        QCOMPARE(*ptr, &obj);
    }

    QCoro::Task<> testThenChained_coro(QCoro::TestContext) {
        SignalTest obj;
        const auto result = co_await qCoro(&obj, &SignalTest::singleArg).then([](const QString &arg) -> QCoro::Task<QString> {
            QTimer timer;
            timer.start(100ms);
            co_await timer;
            co_return arg + arg;
        });

        QCORO_COMPARE(result, QStringLiteral("YAY!YAY!"));
    }

private Q_SLOTS:
    addTest(Triggers)
    addTest(ReturnsValue)
    addTest(ReturnsTuple)
    addTest(TimeoutVoid)
    addTest(TimeoutTriggersVoid)
    addTest(TimeoutValue);
    addTest(TimeoutTriggersValue);
    addTest(TimeoutTuple);
    addTest(TimeoutTriggersTuple);
    addTest(ThenChained)
    addThenTest(Triggers)
    addThenTest(ReturnsValue)
    addThenTest(ReturnsTuple)
};

QTEST_GUILESS_MAIN(QCoroSignalTest)

#include "qcorosignal.moc"
