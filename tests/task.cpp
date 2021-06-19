// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "qcoro/task.h"
#include "qcoro/timer.h"

#include <QTest>
#include <QObject>

#include <chrono>

using namespace std::chrono_literals;

class QCoroTaskTest : public QObject
{
    Q_OBJECT

private:
    QCoro::Task<> testCoroutine() {
        QTimer timer;
        timer.setSingleShot(true);
        timer.start(500ms);
        co_await timer;
    }

    QCoro::Task<QString> testCoroutineValue(const QString &value) {
        co_await testCoroutine();
        co_return value;
    }

private Q_SLOTS:
    void testWaitForVoidCoroutine() {
        QCoro::waitFor(testCoroutine());
    }

    void testWaitForCoroutineWithValue() {
        const auto value = QStringLiteral("Done!");
        const auto result = QCoro::waitFor(testCoroutineValue(value));
        QCOMPARE(result, value);
    }
};

QTEST_GUILESS_MAIN(QCoroTaskTest)

#include "task.moc"
