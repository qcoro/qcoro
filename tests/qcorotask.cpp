// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "testobject.h"
#include "qcorotask.h"
#include "qcorotimer.h"

#include <QTest>
#include <QObject>
#include <QScopeGuard>
#include <QMetaObject>
#include <QTimer>
#include <QRegularExpression>

#include <chrono>
#include <csignal>

using namespace std::chrono_literals;

namespace {

// Used to check whether std::terminate() has been called.
bool terminateHandlerCalled = false;


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
    addThenTest(ImplicitArgumentConversion)

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

    void testRethrowsExceptionWhenCoawaited() {
        QEventLoop el;
        QTimer::singleShot(5s, &el, [&el]() {
            el.quit();
            QFAIL("Timeout waiting for coroutine");
        });

        bool thrown = false;
        QTimer::singleShot(100ms, [&]() -> QCoro::Task<> {
            const auto thrower = []() -> QCoro::Task<> {
                throw std::runtime_error("Ooops, something went wrong.");
            };

            try {
                co_await thrower();
            } catch (const std::runtime_error &e) {
                el.quit();
                thrown = true;
            } catch (const std::exception &) {
                el.quit();
                QCORO_FAIL("Unexpected exception was thrown");
            }
        });

        el.exec();
        QVERIFY(thrown);
    }

    void testAbortsOnUnawaitedExceptionOptionThrowsInEventLoopWhenNotCoawaited_data() {
        QTest::addColumn<bool>("async");
        QTest::newRow("async") << true;
        QTest::newRow("sync") << false;
    }

    void testAbortsOnUnawaitedExceptionOptionThrowsInEventLoopWhenNotCoawaited() {
        QFETCH(bool, async);
        QEventLoop el;

        const auto thrower = [&el, async]() -> QCoro::Task<void, QCoro::TaskOptions<QCoro::Options::AbortOnException>> {
            if (async) {
                co_await timer(); // Make it an asynchronous coroutine
            }
            el.quit(); // We don't actually terminate the program in test mode, so make sure the event loop
                       // ends after we've thrown the exception.
            throw std::runtime_error("Ooops, something went wrong.");
        };

        QTimer::singleShot(100ms, thrower);

        auto oldHandler = QCoro::detail::qcoroSetAbortHandler([]() {
            terminateHandlerCalled = true;
        });
        const auto handlerGuard = qScopeGuard([oldHandler]() {
            QCoro::detail::qcoroSetAbortHandler(oldHandler);
        });

        QTest::ignoreMessage(QtCriticalMsg, QRegularExpression(QStringLiteral(R"(^A QCoro coroutine which wasn't being co_awaited has thrown an unhandled exception.\.*)")));
        el.exec();
        QVERIFY(terminateHandlerCalled);
    }
};

QTEST_GUILESS_MAIN(QCoroTaskTest)

#include "qcorotask.moc"
