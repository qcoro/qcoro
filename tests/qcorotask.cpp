// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "testobject.h"
#include "qcorotask.h"
#include "qcorotimer.h"
#include "qcorosignal.h"

#include <QTest>
#include <QObject>
#include <QScopeGuard>
#include <QMetaObject>
#include <QTimer>

#include <chrono>

using namespace std::chrono_literals;

namespace {

QCoro::Task<> timer(std::chrono::milliseconds timeout = 10ms) {
    QTimer timer;
    timer.setSingleShot(true);
    timer.start(timeout);
    co_await timer;
}

template<typename T>
QCoro::Task<T> timerWithValue(T value, std::chrono::milliseconds timeout = 10ms) {
    co_await timer(timeout);
    co_return value;
}

auto thenScopeTestFunc(QEventLoop *el) {
    return timer().then([el]() {
        el->quit();
    });
}

template<typename T>
QCoro::Task<T> thenScopeTestFuncWithValue(T value) {
    return timer().then([value]() {
        return value;
    });
}

class ImplicitConversionBar {
public:
    int number;
};
class ImplicitConversionFoo {
public:
    ImplicitConversionFoo();
    ImplicitConversionFoo(ImplicitConversionBar bar)
        : string(QString::number(bar.number)) {}

    QString string;
};

struct TestAwaitableBase {
    std::chrono::milliseconds delay() const { return mDelay; }
private:
    std::chrono::milliseconds mDelay = 100ms;
};

template<typename T>
struct TestAwaitable : TestAwaitableBase {
public:
    TestAwaitable(T val)
        : mResult(val)
    {}

    bool await_ready() const { return false; }
    void await_suspend(std::coroutine_handle<> handle) {
        QTimer::singleShot(100ms, [handle = std::move(handle)]() {
            handle.resume();
        });
    }
    T await_resume() {
        return mResult;
    }

private:
    T mResult;
};

template<>
struct TestAwaitable<void> : TestAwaitableBase {
public:
    bool await_ready() const { return false; }
    void await_suspend(std::coroutine_handle<> handle) {
        QTimer::singleShot(100ms, [handle = std::move(handle)]() {
            handle.resume();
        });
    }
    void await_resume() {}
};

template<typename T>
struct TestAwaitableWithCoAwait {
    TestAwaitableWithCoAwait(T val)
        : mResult(val)
    {}

    TestAwaitable<T> operator co_await() {
        return TestAwaitable(mResult);
    }

private:
    T mResult;
};

template<>
struct TestAwaitableWithCoAwait<void> {
    TestAwaitable<void> operator co_await() {
        return TestAwaitable<void>();
    }
};

} // namespace

class QCoroTaskTest : public QCoro::TestObject<QCoroTaskTest>
{
    Q_OBJECT

private:
    template<typename Coro>
    void ignoreCoroutineResult(QEventLoop &el, Coro &&coro) {
        QTimer::singleShot(5s, &el, [&el]() mutable { el.exit(1); });

        coro();
        const int timeout = el.exec();
        QCOMPARE(timeout, 0);
    }

    QCoro::Task<> testSimpleCoroutine_coro(QCoro::TestContext) {
        co_await timer();
    }

    QCoro::Task<> testCoroutineValue_coro(QCoro::TestContext) {
        const auto coro = [](const QString &result) -> QCoro::Task<QString> {
            co_await timer();
            co_return result;
        };

        const auto value = QStringLiteral("Done!");
        const auto result = co_await coro(value);
        QCORO_COMPARE(result, value);
    }

    QCoro::Task<> testCoroutineMoveValue_coro(QCoro::TestContext) {
        const auto coro = [](const QString &result) -> QCoro::Task<std::unique_ptr<QString>> {
            co_await timer();
            co_return std::make_unique<QString>(result);
        };

        const auto value = QStringLiteral("Done ptr!");
        const auto result = co_await coro(value);
        QCORO_COMPARE(*result.get(), value);
    }

    QCoro::Task<> testSyncCoroutine_coro(QCoro::TestContext context) {
        context.setShouldNotSuspend();

        const auto coro = []() -> QCoro::Task<int> {
            co_return 42;
        };

        const auto result = co_await coro();
        QCORO_COMPARE(result, 42);
    }

    QCoro::Task<> testCoroutineWithException_coro(QCoro::TestContext) {
        const auto coro = []() -> QCoro::Task<int> {
            co_await timer();
            throw std::runtime_error("Invalid result");
            co_return 42;
        };

        try {
            const auto result = co_await coro();
            QCORO_FAIL("Exception was not propagated.");
            Q_UNUSED(result);
        } catch (const std::runtime_error &) {
            // OK
        } catch (...) {
            QCORO_FAIL("Exception type was not propagated, or other exception was thrown.");
        }
    }

    QCoro::Task<> testVoidCoroutineWithException_coro(QCoro::TestContext) {
        const auto coro = []() -> QCoro::Task<> {
            co_await timer();
            throw std::runtime_error("Error");
        };

        try {
            co_await coro();
            QCORO_FAIL("Exception was not propagated.");
        } catch (const std::runtime_error &) {
            // OK
        } catch (...) {
            QCORO_FAIL("Exception type was not propagated, or other exception was thrown.");
        }
    }

    QCoro::Task<> testCoroutineFrameDestroyed_coro(QCoro::TestContext) {
        bool destroyed = false;
        const auto coro = [&destroyed]() -> QCoro::Task<> {
            const auto guard = qScopeGuard([&destroyed]() mutable {
                destroyed = true;
            });

            QCORO_VERIFY(!destroyed);
            co_await timer();
            QCORO_VERIFY(!destroyed);
        };

        co_await coro();
        QCORO_VERIFY(destroyed);
    }

    QCoro::Task<> testExceptionPropagation_coro(QCoro::TestContext) {
        QCORO_VERIFY_EXCEPTION_THROWN(
            co_await []() -> QCoro::Task<int> {
                throw std::runtime_error("Test!");
                co_return 42;
            }(),
            std::runtime_error);

        QCORO_VERIFY_EXCEPTION_THROWN(
            co_await []() -> QCoro::Task<> {
                throw std::runtime_error("Test!");
            }(),
            std::runtime_error);

        QCORO_VERIFY_EXCEPTION_THROWN(
            co_await []() -> QCoro::Task<int> {
                co_await timer();
                throw std::runtime_error("Test!");
                co_return 42;
            }(),
            std::runtime_error);

        QCORO_VERIFY_EXCEPTION_THROWN(
            co_await []() -> QCoro::Task<> {
                co_await timer();
                throw std::runtime_error("Test!");
            }(),
            std::runtime_error);
    }

    QCoro::Task<> testThenReturnValueNoArgument_coro(QCoro::TestContext) {
        auto task = timer().then([]() {
            return 42;
        });
        static_assert(std::is_same_v<decltype(task), QCoro::Task<int>>);
        const auto result = co_await task;
        QCORO_COMPARE(result, 42);
    }

    QCoro::Task<> testThenReturnValueWithArgument_coro(QCoro::TestContext) {
        auto task = timerWithValue(42).then([](int param) -> QCoro::Task<int> {
            co_return param * 2;
        });

        static_assert(std::is_same_v<decltype(task), QCoro::Task<int>>);
        const auto result = co_await task;
        QCORO_COMPARE(result, 84);
    }

    QCoro::Task<> testThenReturnTaskVoidNoArgument_coro(QCoro::TestContext) {
        auto task = timer().then([]() -> QCoro::Task<void> {
            co_await timer();
        });
        static_assert(std::is_same_v<decltype(task), QCoro::Task<void>>);
        co_await task;
    }

    QCoro::Task<> testThenReturnTaskVoidWithArgument_coro(QCoro::TestContext) {
        auto task = timerWithValue(42).then([](int result) -> QCoro::Task<void> {
            co_await timer();
            Q_UNUSED(result);
        });
        static_assert(std::is_same_v<decltype(task), QCoro::Task<void>>);
        co_await task;
    }

    QCoro::Task<> testThenReturnTaskTNoArgument_coro(QCoro::TestContext) {
        auto task = timer().then([]() -> QCoro::Task<int> {
            co_await timer();
            co_return 42;
        });
        static_assert(std::is_same_v<decltype(task), QCoro::Task<int>>);
        const auto result = co_await task;
        QCORO_COMPARE(result, 42);
    }

    QCoro::Task<> testThenReturnTaskTWithArgument_coro(QCoro::TestContext) {
        auto task = timerWithValue(42).then([](int val) -> QCoro::Task<int> {
            co_await timer();
            co_return val * 2;
        });
        static_assert(std::is_same_v<decltype(task), QCoro::Task<int>>);
        const auto result = co_await task;
        QCORO_COMPARE(result, 84);
    }

    QCoro::Task<> testThenReturnValueSync_coro(QCoro::TestContext context) {
        context.setShouldNotSuspend();

        auto task = []() -> QCoro::Task<int> {
            co_return 42;
        }().then([](int param) {
            return param * 2;
        });
        const int result = co_await task;
        QCORO_COMPARE(result, 84);
    }

    QCoro::Task<> testThenScopeAwait_coro(QCoro::TestContext) {
        const int result = co_await thenScopeTestFuncWithValue(42);
        QCORO_COMPARE(result, 42);
    }

    QCoro::Task<> testThenExceptionPropagation_coro(QCoro::TestContext) {
        QCORO_VERIFY_EXCEPTION_THROWN(
            co_await []() -> QCoro::Task<int> {
                co_await timer();
                throw std::runtime_error("Test!");
                co_return 42;
            }().then([](int) -> QCoro::Task<> {
                QCORO_FAIL("The then() callback should never be called");
                co_return;
            }),
            std::runtime_error);
    }

    QCoro::Task<> testThenError_coro(QCoro::TestContext) {
        bool exceptionThrown = false;

        co_await []() -> QCoro::Task<int> {
            co_await timer();
            throw std::runtime_error("Test!");
            co_return 42;
        }().then([](int) -> QCoro::Task<> {
                QCORO_FAIL("The then() callback should not be called");
            },
            [&exceptionThrown](const std::exception &) {
                exceptionThrown = true;
            }
        );

        QCORO_VERIFY(exceptionThrown);
    }

    QCoro::Task<> testThenErrorWithValue_coro(QCoro::TestContext) {
        bool exceptionThrown = false;
        bool thenCalled = false;

        const int result = co_await []() -> QCoro::Task<> {
            co_await timer();
            throw std::runtime_error("Test!");
        }().then([&thenCalled]() -> QCoro::Task<int> {
                thenCalled = true;
                co_return 42;
            },
            [&exceptionThrown](const std::exception &) {
                exceptionThrown = true;
            }
        );

        // We handled an exception, so there's no error and it should
        // be default-constructed.
        QCORO_COMPARE(result, 0);
        QCORO_VERIFY(!thenCalled);
        QCORO_VERIFY(exceptionThrown);
    }

    void testThenImplicitArgumentConversion_coro(TestLoop &el) {
        QTimer test;
        QString result;
        qCoro(test).waitForTimeout().then([]() -> QCoro::Task<ImplicitConversionBar> {
            ImplicitConversionBar bar{42};
            co_await timer(10ms);
            co_return bar;
        }).then([&](ImplicitConversionFoo foo) {
            result = foo.string;
            el.quit();
        });
        test.start(10ms);
        el.exec();

        QCOMPARE(result, QStringLiteral("42"));
    }

    QCoro::Task<> testMultipleAwaiters_coro(QCoro::TestContext) {
        auto task = timer(100ms);

        bool called = false;
        // Internally co_awaits task
        task.then([&called]() {
            called = true;
        });

        co_await task;

        QCORO_VERIFY(called);
    }

    QCoro::Task<> testMultipleAwaitersSync_coro(QCoro::TestContext ctx) {
        ctx.setShouldNotSuspend();

        auto task = []() -> QCoro::Task<> { co_return; }();

        bool called = false;
        task.then([&called]() {
            called = true;
        });

        co_await task;

        QCORO_VERIFY(called);
    }

    Q_SIGNAL void callbackCalled();

    template <typename QObjectDerived, typename Signal>
    QCoro::Task<> verifySignalEmitted(QObjectDerived *context, Signal &&signal) {
        bool called = false;
        co_await qCoro(context, std::move(signal)).then([&]() {
            called = true;
        });
        QCORO_VERIFY(called);
    }

    QCoro::Task<> testTaskConnect_coro(QCoro::TestContext) {
        // Test that free functions can be passed as callback
        QCoro::connect(timer(), this, [this]() {
            Q_EMIT callbackCalled();
        });
        co_await verifySignalEmitted(this, &QCoroTaskTest::callbackCalled);

        // Check that member functions can be passed as callback
        QCoro::connect(timer(), this, &QCoroTaskTest::callbackCalled);
        co_await verifySignalEmitted(this, &QCoroTaskTest::callbackCalled);

        // Test that the code still compiles if the value of the coroutine is not used by the function.
        auto nonVoidCoroutine = []() -> QCoro::Task<QString> {
            co_await timer();
            co_return QStringLiteral("Hello World!");
        };
        QCoro::connect(nonVoidCoroutine(), this, [this]() {
            Q_EMIT callbackCalled();
        });
        co_await verifySignalEmitted(this, &QCoroTaskTest::callbackCalled);

        QCoro::connect(nonVoidCoroutine(), this, [this](QString) {
            Q_EMIT callbackCalled();
        });
        co_await verifySignalEmitted(this, &QCoroTaskTest::callbackCalled);
    }

private Q_SLOTS:
    addTest(SimpleCoroutine)
    addTest(CoroutineValue)
    addTest(CoroutineMoveValue)
    addTest(SyncCoroutine)
    addTest(CoroutineWithException)
    addTest(VoidCoroutineWithException)
    addTest(CoroutineFrameDestroyed)
    addTest(ExceptionPropagation)
    addTest(ThenReturnValueNoArgument)
    addTest(ThenReturnValueWithArgument)
    addTest(ThenReturnTaskVoidNoArgument)
    addTest(ThenReturnTaskVoidWithArgument)
    addTest(ThenReturnTaskTNoArgument)
    addTest(ThenReturnTaskTWithArgument)
    addTest(ThenReturnValueSync)
    addTest(ThenScopeAwait)
    addTest(ThenExceptionPropagation)
    addTest(ThenError)
    addTest(ThenErrorWithValue)
    addTest(TaskConnect)
    addThenTest(ImplicitArgumentConversion)
    addTest(MultipleAwaiters)
    addTest(MultipleAwaitersSync)

    // See https://github.com/danvratil/qcoro/issues/24
    void testEarlyReturn()
    {
        QEventLoop loop;

        const auto testReturn = [](bool immediate) -> QCoro::Task<bool> {
            if (immediate) {
                co_return true;
            } else {
                co_await timer();
                co_return true;
            }
        };

        bool immediateResult = false;
        bool delayedResult = false;

        const auto testImmediate = [&]() -> QCoro::Task<> {
            immediateResult = co_await testReturn(true);
        };

        const auto testDelayed = [&]() -> QCoro::Task<> {
            delayedResult = co_await testReturn(false);
            loop.quit();
        };

        QMetaObject::invokeMethod(
            &loop, [&]() { testImmediate(); }, Qt::QueuedConnection);
        QMetaObject::invokeMethod(
            &loop, [&]() { testDelayed(); }, Qt::QueuedConnection);

        loop.exec();

        QVERIFY(immediateResult);
        QVERIFY(delayedResult);
    }

    // TODO: Test timeout
    void testWaitFor() {
        QCoro::waitFor(timer());
    }

    // TODO: Test timeout
    void testWaitForWithValue() {
        const auto result = QCoro::waitFor([]() -> QCoro::Task<int> {
            co_await timer();
            co_return 42;
        }());
        QCOMPARE(result, 42);
    }

    void testEarlyReturnWaitFor() {
        QCoro::waitFor([]() -> QCoro::Task<> { co_return; }());
    }

    void testEarlyReturnWaitForWithValue() {
        const auto result = QCoro::waitFor([]() -> QCoro::Task<int> {
            co_return 42;
        }());
        QCOMPARE(result, 42);
    }

    void testWaitForAwaitable() {
        TestAwaitable<int> awaitable(42);
        QElapsedTimer timer;
        timer.start();

        static_assert(std::is_same_v<decltype(QCoro::waitFor(awaitable)), int>);
        const int result = QCoro::waitFor(awaitable);
        QCOMPARE(result, 42);
        QVERIFY(timer.elapsed() >= static_cast<float>(awaitable.delay().count()) * 0.9);

    }

    void testWaitForVoidAwaitable() {
        TestAwaitable<void> awaitable;
        QElapsedTimer timer;
        timer.start();

        static_assert(std::is_void_v<decltype(QCoro::waitFor(awaitable))>);
        QCoro::waitFor(awaitable);

        QVERIFY(timer.elapsed() >= static_cast<float>(awaitable.delay().count()) * 0.9);
    }

    void testWaitForAwaitableWithOperatorCoAwait() {
        TestAwaitableWithCoAwait<int> awaitable(42);
        QCoro::waitFor(awaitable);
        QElapsedTimer timer;
        timer.start();

        static_assert(std::is_same_v<decltype(QCoro::waitFor(awaitable)), int>);
        const int result = QCoro::waitFor(awaitable);
        QCOMPARE(result, 42);
        QVERIFY(timer.elapsed() >= (90ms).count());
    }

    void testWaitForVoidAwaitableWithOperatorCoAwait() {
        TestAwaitableWithCoAwait<void> awaitable;
        QElapsedTimer timer;
        timer.start();

        static_assert(std::is_void_v<decltype(QCoro::waitFor(awaitable))>);
        QCoro::waitFor(awaitable);

        QVERIFY(timer.elapsed() >= (90ms).count());
    }

    void testWaitForWithValueRethrowsException() {
        const auto coro = []() -> QCoro::Task<int> {
            co_await timer();
            throw std::runtime_error("Exception");
            co_return 42;
        };

#if QT_VERSION >= QT_VERSION_CHECK(6, 3, 0)
        QVERIFY_THROWS_EXCEPTION(std::runtime_error, QCoro::waitFor(coro()));
#else
        QVERIFY_EXCEPTION_THROWN(QCoro::waitFor(coro()), std::runtime_error);
#endif
    }

    void testWaitForRethrowsException() {
        const auto coro = []() -> QCoro::Task<> {
            co_await timer();
            throw std::runtime_error("Exception");
        };

#if QT_VERSION >= QT_VERSION_CHECK(6, 3, 0)
        QVERIFY_THROWS_EXCEPTION(std::runtime_error, QCoro::waitFor(coro()));
#else
        QVERIFY_EXCEPTION_THROWN(QCoro::waitFor(coro()), std::runtime_error);
#endif
    }


    void testIgnoredVoidTaskResult() {
        QEventLoop el;
        ignoreCoroutineResult(el, [&el]() -> QCoro::Task<> {
            co_await timer();
            el.quit();
        });
    }

    void testIgnoredValueTaskResult() {
        QEventLoop el;
        ignoreCoroutineResult(el, [&el]() -> QCoro::Task<QString> {
            co_await timer();
            el.quit();
            co_return QStringLiteral("Result");
        });
    }

    void testThenVoidNoArgument() {
        QEventLoop el;

        {
            timer().then([&el]() {
                el.quit();
            });
        }

        el.exec();
    }

    void testThenDiscardsReturnValue() {
        QEventLoop el;
        bool called = false;

        timerWithValue(42).then([&]() {
            el.quit();
            called = true;
        });

        el.exec();

        QVERIFY(called);
    }

    void testThenScope() {
        QEventLoop el;
        thenScopeTestFunc(&el);
        el.exec();
    }

    void testThenVoidWithArgument() {
        QEventLoop el;
        int result = 0;

        {
            timerWithValue(42).then([&el, &result](int val) {
                result = val;
                el.quit();
            });
        }

        el.exec();
        QCOMPARE(result, 42);
    }

    void testThenVoidWithFunction() {
        QEventLoop el;

        timerWithValue(10ms).then(timer).then([&el]() {
            el.quit();
        });

        el.exec();
    }

    void testThenErrorInCallback() {
        QEventLoop el;
        QTimer::singleShot(5s, &el, [&el]() {
            el.quit();
            QFAIL("Timeout waiting for coroutine");
        });

        []() -> QCoro::Task<> {
            co_await timer();
        }().then([]() {
            throw std::runtime_error("Test!");
        }, [](const std::exception &) {
            QFAIL("Continuation exception should not be handled by the same error handled");
        }).then([]() {
            QFAIL("Second then continuation should not be called.");
        }, [&el](const std::exception &) {
            el.quit();
        });

        el.exec();
    }

    void testThenExceptionInError() {
        QEventLoop el;
        QTimer::singleShot(5s, &el, [&el]() {
            el.quit();
            QFAIL("Timeout waiting for coroutine");
        });

        []() -> QCoro::Task<> {
            co_await timer();
            throw std::runtime_error("Test!");
        }().then([]() {
            QFAIL("The then() continuation should not be called");
        }, [](const std::exception &) {
            throw std::runtime_error("Another test!");
        }).then([]() {
            QFAIL("Second then() continuation should not be called");
        }, [&el](const std::exception &) {
            el.quit();
        });

        el.exec();
    }

    void testTaskConnectContext_coro() {
        auto task = timer(200ms);
        static_assert(std::is_same_v<decltype(task), QCoro::Task<>>);

        bool called = false;
        auto context = new QObject();

        QCoro::connect(task, context, [&]() {
            called = true;
        });

        // Delete context, callback should not be called
        delete context;

        QCoro::waitFor(task);

        QVERIFY(!called);
    }


};

QTEST_GUILESS_MAIN(QCoroTaskTest)

#include "qcorotask.moc"
