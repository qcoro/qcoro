// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "testobject.h"
#include "qcoro/qcoroprocess.h"

#include <QProcess>

class QCoroProcessTest : public QCoro::TestObject<QCoroProcessTest> {
    Q_OBJECT

private:
    QCoro::Task<> testStartTriggers_coro(QCoro::TestContext) {
        QProcess process;

        co_await qCoro(process).start(QStringLiteral("true"), {});

        QCORO_COMPARE(process.state(), QProcess::Running);

        process.waitForFinished();
    }

    QCoro::Task<> testStartNoArgsTriggers_coro(QCoro::TestContext) {
        QProcess process;
        process.setProgram(QStringLiteral("true"));

        co_await qCoro(process).start();

        QCORO_COMPARE(process.state(), QProcess::Running);

        process.waitForFinished();
    }

    QCoro::Task<> testStartDoesntBlock_coro(QCoro::TestContext) {
        QCoro::EventLoopChecker eventLoopResponsive{1, 0ms};

        QProcess process;
        co_await qCoro(process).start(QStringLiteral("true"), {});

        QCORO_VERIFY(eventLoopResponsive);

        process.waitForFinished();
    }

    QCoro::Task<> testStartDoesntCoAwaitRunningProcess_coro(QCoro::TestContext ctx) {
        QProcess process;
#if defined(__GNUC__) && !defined(__clang__)
#pragma message "Workaround for GCC ICE!"
        // Workaround GCC bug https://bugzilla.redhat.com/1952671
        // GCC ICEs at the end of this function due to presence of two co_await statements.
        process.start(QStringLiteral("sleep"), {QStringLiteral("1")});
        process.waitForStarted();
#else
        co_await qCoro(process).start(QStringLiteral("sleep"), {QStringLiteral("1")});
#endif

        QCORO_COMPARE(process.state(), QProcess::Running);

        ctx.setShouldNotSuspend();

        QTest::ignoreMessage(QtWarningMsg, "QProcess::start: Process is already running");
        co_await qCoro(process).start();

        process.waitForFinished();
    }

    QCoro::Task<> testFinishTriggers_coro(QCoro::TestContext) {
        QProcess process;
        process.start(QStringLiteral("sleep"), {QStringLiteral("1")});
        process.waitForStarted();

        QCORO_COMPARE(process.state(), QProcess::Running);

        const auto ok = co_await qCoro(process).waitForFinished();

        QCORO_VERIFY(ok);
        QCORO_COMPARE(process.state(), QProcess::NotRunning);
    }

    QCoro::Task<> testFinishDoesntCoAwaitFinishedProcess_coro(QCoro::TestContext ctx) {
        QProcess process;
        process.start(QStringLiteral("true"), QStringList{});
        process.waitForFinished();

        ctx.setShouldNotSuspend();

        const auto ok = co_await qCoro(process).waitForFinished();
        QCORO_VERIFY(ok);
    }

    QCoro::Task<> testFinishCoAwaitTimeout_coro(QCoro::TestContext) {
        QProcess process;
        process.start(QStringLiteral("sleep"), {QStringLiteral("2")});
        process.waitForStarted();

        QCORO_COMPARE(process.state(), QProcess::Running);

        const auto ok = co_await qCoro(process).waitForFinished(1s);

        QCORO_VERIFY(!ok);
        QCORO_COMPARE(process.state(), QProcess::Running);

        process.waitForFinished();
    }

private Q_SLOTS:
    addTest(StartTriggers)
    addTest(StartNoArgsTriggers)
    addTest(StartDoesntBlock)
    addTest(StartDoesntCoAwaitRunningProcess)
    addTest(FinishTriggers)
    addTest(FinishDoesntCoAwaitFinishedProcess)
    addTest(FinishCoAwaitTimeout)
};

QTEST_GUILESS_MAIN(QCoroProcessTest)

#include "qcoroprocess.moc"
