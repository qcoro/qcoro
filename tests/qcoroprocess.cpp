// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "testobject.h"

#include "qcoro/core/qcoroprocess.h"

#include <QProcess>

#ifdef Q_OS_WIN
// There's no equivalent to "true" command on Windows, so do a single ping to localhost instead,
// which terminates almost immediately.
#define DUMMY_EXEC QStringLiteral("ping")
#define DUMMY_ARGS                                                                                 \
    { QStringLiteral("127.0.0.1"), QStringLiteral("-n"), QStringLiteral("1") }
// On windows, the equivalent to Linux "sleep" is "timeout", but it fails due to QProcess redirecting
// stdin, which "timeout" doesn't support (it waits for keypress to interrupt). However, "ping" pings
// every second, so specifying number of pings to the desired timeout makes it behave basically like
// the Linux "sleep".
#define SLEEP_EXEC QStringLiteral("ping")
#define SLEEP_ARGS(timeout)                                                                        \
    { QStringLiteral("127.0.0.1"), QStringLiteral("-n"), QString::number(timeout) }

#else
#define DUMMY_EXEC QStringLiteral("true")
#define DUMMY_ARGS {}
#define SLEEP_EXEC QStringLiteral("sleep")
#define SLEEP_ARGS(timeout) { QString::number(timeout) }
#endif

class QCoroProcessTest : public QCoro::TestObject<QCoroProcessTest> {
    Q_OBJECT

private:
    QCoro::Task<> testStartTriggers_coro(QCoro::TestContext context) {
#ifdef Q_OS_WIN
        // QProcess::start() on Windows is synchronous, despite what the documentation says,
        // so the coroutine will not suspend.
        context.setShouldNotSuspend();
#else
        Q_UNUSED(context);
#endif

        QProcess process;
        const bool ok = co_await qCoro(process).start(DUMMY_EXEC, DUMMY_ARGS);

        QCORO_VERIFY(ok);
        QCORO_COMPARE(process.state(), QProcess::Running);

        process.waitForFinished();
    }

    void testThenStartTriggers_coro(TestLoop &el) {
        QProcess process;
        bool called = false;
        qCoro(process).start(DUMMY_EXEC, DUMMY_ARGS).then([&](bool started) {
            called = true;
            el.quit();
            QVERIFY(started);
            QCOMPARE(process.state(), QProcess::Running);
        });
        el.exec();
        QVERIFY(called);

        process.waitForFinished();
    }

    QCoro::Task<> testStartNoArgsTriggers_coro(QCoro::TestContext context) {
#ifdef Q_OS_WIN
        context.setShouldNotSuspend();
#else
        Q_UNUSED(context);
#endif

        QProcess process;
        process.setProgram(DUMMY_EXEC);
        process.setArguments(DUMMY_ARGS);

        const bool ok = co_await qCoro(process).start();

        QCORO_VERIFY(ok);
        QCORO_COMPARE(process.state(), QProcess::Running);

        process.waitForFinished();
    }

    void testThenStartNoArgsTriggers_coro(TestLoop &el) {
        QProcess process;
        process.setProgram(DUMMY_EXEC);
        process.setArguments(DUMMY_ARGS);

        bool called = false;
        qCoro(process).start().then([&](bool started) {
            called = true;
            el.quit();
            QVERIFY(started);
            QCOMPARE(process.state(), QProcess::Running);
        });
        el.exec();
        QVERIFY(called);

        process.waitForFinished();
    }

    QCoro::Task<> testStartDoesntBlock_coro(QCoro::TestContext) {
        QCoro::EventLoopChecker eventLoopResponsive{1, 0ms};

        QProcess process;
        const bool ok = co_await qCoro(process).start(DUMMY_EXEC, DUMMY_ARGS);

        QCORO_VERIFY(ok);
        QCORO_VERIFY(eventLoopResponsive);

        process.waitForFinished();
    }

    QCoro::Task<> testStartDoesntCoAwaitRunningProcess_coro(QCoro::TestContext ctx) {
        QProcess process;
#if defined(__GNUC__) && !defined(__clang__)
#pragma message "Workaround for GCC ICE!"
        // Workaround GCC bug https://bugzilla.redhat.com/1952671
        // GCC ICEs at the end of this function due to presence of two co_await statements.
        process.start(SLEEP_EXEC, SLEEP_ARGS(1));
        process.waitForStarted();
#else
        const bool ok = co_await qCoro(process).start(SLEEP_EXEC, SLEEP_ARGS(1));
        QCORO_VERIFY(ok);
#endif

        QCORO_COMPARE(process.state(), QProcess::Running);

        ctx.setShouldNotSuspend();

        QTest::ignoreMessage(QtWarningMsg, "QProcess::start: Process is already running");
        co_await qCoro(process).start();

        process.waitForFinished();
    }

    QCoro::Task<> testFinishTriggers_coro(QCoro::TestContext) {
        QProcess process;
        process.start(SLEEP_EXEC, SLEEP_ARGS(1));
        process.waitForStarted();

        QCORO_COMPARE(process.state(), QProcess::Running);

        const auto ok = co_await qCoro(process).waitForFinished();

        QCORO_VERIFY(ok);
        QCORO_COMPARE(process.state(), QProcess::NotRunning);
    }

    void testThenFinishTriggers_coro(TestLoop &el) {
        QProcess process;
        process.start(SLEEP_EXEC, SLEEP_ARGS(1));
        process.waitForStarted();

        QCOMPARE(process.state(), QProcess::Running);

        bool called = false;
        qCoro(process).waitForFinished().then([&](bool finished) {
            called = true;
            el.quit();
            QVERIFY(finished);

            QCOMPARE(process.state(), QProcess::NotRunning);
            QCOMPARE(process.exitStatus(), QProcess::NormalExit);
        });
        el.exec();
        QVERIFY(called);
    }

    QCoro::Task<> testFinishDoesntCoAwaitFinishedProcess_coro(QCoro::TestContext ctx) {
        QProcess process;
        process.start(DUMMY_EXEC, QStringList DUMMY_ARGS);
        process.waitForFinished();

        ctx.setShouldNotSuspend();

        const auto ok = co_await qCoro(process).waitForFinished();
        QCORO_VERIFY(!ok);
    }

    QCoro::Task<> testFinishCoAwaitTimeout_coro(QCoro::TestContext) {
        QProcess process;

        process.start(SLEEP_EXEC, SLEEP_ARGS(2));
        process.waitForStarted();

        QCORO_COMPARE(process.state(), QProcess::Running);

        const auto ok = co_await qCoro(process).waitForFinished(1s);

        QCORO_VERIFY(!ok);
        QCORO_COMPARE(process.state(), QProcess::Running);

        process.waitForFinished();
    }

    void testThenFinishCoAwaitTimeout_coro(TestLoop &el) {
        QProcess process;
        process.start(SLEEP_EXEC, SLEEP_ARGS(2));
        process.waitForStarted();

        QCOMPARE(process.state(), QProcess::Running);
        bool called = false;
        qCoro(process).waitForFinished(1s).then([&](bool finished) {
            called = true;
            el.quit();
            QVERIFY(!finished);
        });
        el.exec();
        QVERIFY(called);

        process.waitForFinished();
    }

private Q_SLOTS:
    addCoroAndThenTests(StartTriggers)
    addCoroAndThenTests(StartNoArgsTriggers)
#ifndef Q_OS_WIN // start always blocks on Windows
    addTest(StartDoesntBlock)
#endif
    addTest(StartDoesntCoAwaitRunningProcess)
    addCoroAndThenTests(FinishTriggers)
    addTest(FinishDoesntCoAwaitFinishedProcess)
    addCoroAndThenTests(FinishCoAwaitTimeout)
};

QTEST_GUILESS_MAIN(QCoroProcessTest)

#include "qcoroprocess.moc"
