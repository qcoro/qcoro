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
    explicit SignalTest(bool active = true) {
        if (active) {
            QTimer::singleShot(100ms, this, &SignalTest::emit);
        }
    }

    void emit() {
        Q_EMIT voidSignal();
        Q_EMIT singleArg(QStringLiteral("YAY!"));
        Q_EMIT multiArg(QStringLiteral("YAY!"), 42, this);
        Q_EMIT privateVoid(QPrivateSignal{});
        Q_EMIT privateSingleArg(QStringLiteral("YAY!"), QPrivateSignal{});
        Q_EMIT privateMultiArg(QStringLiteral("YAY!"), 42, this, QPrivateSignal{});
    }

Q_SIGNALS:
    void voidSignal();
    void singleArg(const QString &);
    void multiArg(const QString &, int, QObject *);
    void privateVoid(QPrivateSignal);
    void privateSingleArg(const QString &, QPrivateSignal);
    void privateMultiArg(const QString &, int, QObject *, QPrivateSignal);
};

class MultiSignalTest : public SignalTest {
    Q_OBJECT
public:
    explicit MultiSignalTest(bool active = true)
        : SignalTest(false) {
        if (active) {
            mTimer.setInterval(10ms);
            connect(&mTimer, &QTimer::timeout, this, &MultiSignalTest::emit);
            mTimer.start();
        }
    }

private:
    QTimer mTimer;
};


class SimpleSignal: public QObject {
    Q_OBJECT
public:
    void send(int id) {
        Q_EMIT messageReceived(id);
    }

    Q_SIGNAL void messageReceived(int id);

    QCoro::Task<int> waitForMessage(int id) {
        QCORO_FOREACH(int msgId, qCoroSignalListener(this, &SimpleSignal::messageReceived)) {
            if (msgId == id) {
                co_return id;
            }
        }

        co_return -1;
    }
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
        static_assert(std::is_same_v<decltype(result), const std::tuple<QString, int, QObject *>>);
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
                const std::optional<std::tuple<QString, int, QObject *>>>);
        QCORO_VERIFY(!result.has_value());
    }

    QCoro::Task<> testTimeoutTuple_coro(QCoro::TestContext) {
        SignalTest obj;

        const auto result = co_await qCoro(&obj, &SignalTest::multiArg, 1s);
        static_assert(std::is_same_v<
                decltype(result),
                const std::optional<std::tuple<QString, int, QObject *>>>);
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

    QCoro::Task<> testVoidQPrivateSignal_coro(QCoro::TestContext) {
        SignalTest obj;

        const auto result = co_await qCoro(&obj, &SignalTest::privateVoid);
        static_assert(std::is_same_v<decltype(result), const std::tuple<>>);
        Q_UNUSED(result);
    }

    QCoro::Task<> testSingleArgQPrivateSignal_coro(QCoro::TestContext) {
        SignalTest obj;

        const auto result = co_await qCoro(&obj, &SignalTest::privateSingleArg);
        static_assert(std::is_same_v<decltype(result), const QString>);
        QCORO_COMPARE(result, QStringLiteral("YAY!"));
    }

    QCoro::Task<> testMultiArgQPrivateSignal_coro(QCoro::TestContext) {
        SignalTest obj;

        const auto [str, num, ptr] = co_await qCoro(&obj, &SignalTest::privateMultiArg);
        static_assert(std::is_same_v<decltype(str), const QString>);
        static_assert(std::is_same_v<decltype(num), const int>);
        static_assert(std::is_same_v<decltype(ptr), QObject * const>);
        QCORO_COMPARE(str, QStringLiteral("YAY!"));
        QCORO_COMPARE(num, 42);
        QCORO_COMPARE(ptr, &obj);
    }

    QCoro::Task<> testSignalListenerVoid_coro(QCoro::TestContext) {
        MultiSignalTest obj;

        auto generator = qCoroSignalListener(&obj, &MultiSignalTest::voidSignal);
        int count = 0;
        QCORO_FOREACH(const std::tuple<> &value, generator) {
            Q_UNUSED(value);
            if (++count == 10) {
                break;
            }
        }

        QCORO_COMPARE(count, 10);
    }

    QCoro::Task<> testSignalListenerValue_coro(QCoro::TestContext) {
        MultiSignalTest obj;

        auto generator = qCoroSignalListener(&obj, &MultiSignalTest::singleArg);
        int count = 0;
        QCORO_FOREACH(const QString &value, generator) {
            QCORO_COMPARE(value, QStringLiteral("YAY!"));
            if (++count == 10) {
                break;
            }
        }

        QCORO_COMPARE(count, 10);
    }

    QCoro::Task<> testSignalListenerTuple_coro(QCoro::TestContext) {
        MultiSignalTest obj;

        auto generator = qCoroSignalListener(&obj, &MultiSignalTest::multiArg);
        int count = 0;
        QCORO_FOREACH(const auto &value, generator) {
            QCORO_COMPARE(std::get<0>(value), QStringLiteral("YAY!"));
            QCORO_COMPARE(std::get<1>(value), 42);
            QCORO_COMPARE(std::get<2>(value), &obj);
            if (++count == 10) {
                break;
            }
        }

        QCORO_COMPARE(count, 10);
    }

    QCoro::Task<> testSignalListenerTimeout_coro(QCoro::TestContext) {
        QObject obj;

        // A signal that doesn't get invoked
        auto generator = qCoroSignalListener(&obj, &QObject::destroyed, 1ms);
        QCORO_FOREACH(const auto &value, generator) {
            Q_UNUSED(value);
            QCORO_FAIL("The signal should time out and the generator should not return invalid iterator.");
        }
    }

    QCoro::Task<> testSignalListenerQueue_coro(QCoro::TestContext ctx) {
        SignalTest test{false};
        // I have a generator
        auto generator = qCoroSignalListener(&test, &SignalTest::voidSignal);
        // I emit signals that the generator is listening to, the generator
        // should enqueue them.
        for (int i = 0; i < 10; ++i) {
            test.emit();
        }

        // I asynchronously wait for first iterator
        auto it = co_await generator.begin();
        int count = 0;
        ctx.setShouldNotSuspend();
        // I loop over generator - this should not suspend as we are simply consuming
        // events from the queue.
        for (; it != generator.end(); co_await ++it) {
            if (++count == 10) {
                break;
            }
        }
        QCORO_COMPARE(count, 10);
    }

    QCoro::Task<> testSignalAfterListenerQuits_coro(QCoro::TestContext) {
        SimpleSignal simple;
        auto msg1 = simple.waitForMessage(1);
        auto msg2 = simple.waitForMessage(2);
        simple.send(1);
        simple.send(2);
        QCORO_COMPARE(co_await msg1, 1);
        QCORO_COMPARE(co_await msg2, 2);
    }

    QCoro::Task<> testSignalListenerQPrivateSignalVoid_coro(QCoro::TestContext) {
        MultiSignalTest obj;

        auto generator = qCoroSignalListener(&obj, &MultiSignalTest::privateVoid);
        int count = 0;
        QCORO_FOREACH(const auto &value, generator) {
            static_assert(std::is_same_v<decltype(value), const std::tuple<> &>);
            Q_UNUSED(value);
            if (++count == 10) {
                break;
            }
        }

        QCORO_COMPARE(count, 10);
    }

    QCoro::Task<> testSignalListenerQPrivateSignalValue_coro(QCoro::TestContext) {
         MultiSignalTest obj;

        auto generator = qCoroSignalListener(&obj, &MultiSignalTest::privateSingleArg);
        int count = 0;
        QCORO_FOREACH(const auto &value, generator) {
            static_assert(std::is_same_v<decltype(value), const QString &>);
            QCORO_COMPARE(value, QStringLiteral("YAY!"));
            if (++count == 10) {
                break;
            }
        }

        QCORO_COMPARE(count, 10);
    }

    QCoro::Task<> testSignalListenerQPrivateSignalTuple_coro(QCoro::TestContext) {
        MultiSignalTest obj;

        auto generator = qCoroSignalListener(&obj, &MultiSignalTest::privateMultiArg);
        int count = 0;
        QCORO_FOREACH(const auto &value, generator) {
            static_assert(std::is_same_v<decltype(value), const std::tuple<QString, int, QObject *> &>);
            QCORO_COMPARE(std::get<QString>(value), QStringLiteral("YAY!"));
            QCORO_COMPARE(std::get<int>(value), 42);
            QCORO_COMPARE(std::get<QObject *>(value), &obj);
            if (++count == 10) {
                break;
            }
        }

        QCORO_COMPARE(count, 10);
    }

private Q_SLOTS:
    addTest(Triggers)
    addTest(ReturnsValue)
    addTest(ReturnsTuple)
    addTest(TimeoutVoid)
    addTest(TimeoutTriggersVoid)
    addTest(TimeoutValue)
    addTest(TimeoutTriggersValue)
    addTest(TimeoutTuple)
    addTest(TimeoutTriggersTuple)
    addTest(ThenChained)
    addTest(VoidQPrivateSignal)
    addTest(SingleArgQPrivateSignal)
    addTest(MultiArgQPrivateSignal)
    addThenTest(Triggers)
    addThenTest(ReturnsValue)
    addThenTest(ReturnsTuple)
    addTest(SignalListenerVoid)
    addTest(SignalListenerValue)
    addTest(SignalListenerTuple)
    addTest(SignalListenerTimeout)
    addTest(SignalListenerQueue)
    addTest(SignalAfterListenerQuits)
    addTest(SignalListenerQPrivateSignalVoid)
    addTest(SignalListenerQPrivateSignalValue)
    addTest(SignalListenerQPrivateSignalTuple)
};

QTEST_GUILESS_MAIN(QCoroSignalTest)

#include "qcorosignal.moc"
