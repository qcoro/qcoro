// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "testobject.h"
#include "qcoro/coro.h"

#include <QTimer>

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


class QCoroSignalTest: public QCoro::TestObject<QCoroSignalTest>
{
    Q_OBJECT

private:
    QCoro::Task<> testTriggers_coro(QCoro::TestContext) {
        SignalTest obj;

        co_await qCoro(&obj, &SignalTest::voidSignal);
        static_assert(std::is_void_v<decltype(qCoro(&obj, &SignalTest::voidSignal).await_resume())>);
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

private Q_SLOTS:
    addTest(Triggers)
    addTest(ReturnsValue)
    addTest(ReturnsTuple)
};

QTEST_GUILESS_MAIN(QCoroSignalTest)

#include "qcorosignal.moc"

