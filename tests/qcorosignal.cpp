// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "testobject.h"

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
};

QTEST_GUILESS_MAIN(QCoroSignalTest)

#include "qcorosignal.moc"
