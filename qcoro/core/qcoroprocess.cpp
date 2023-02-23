// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include <QtGlobal>

#if QT_CONFIG(process)

#include "qcoroprocess.h"
#include "qcorosignal.h"

#include <QProcess>

using namespace QCoro::detail;

QCoroProcess::QCoroProcess(QProcess *process)
    : QCoroIODevice(process)
{}

QCoro::Task<bool> QCoroProcess::waitForStarted(int timeout_msecs) {
    return waitForStarted(std::chrono::milliseconds{timeout_msecs});
}

QCoro::Task<bool> QCoroProcess::waitForStarted(std::chrono::milliseconds timeout) {
    const auto *process = qobject_cast<QProcess *>(mDevice.data());
    if (process->state() == QProcess::Starting) {
        const auto started = co_await qCoro(process, &QProcess::started, timeout);
        co_return started.has_value();
    }

    co_return process->state() == QProcess::Running;
}

QCoro::Task<bool> QCoroProcess::waitForFinished(int timeout_msecs) {
    return waitForFinished(std::chrono::milliseconds{timeout_msecs});
}

QCoro::Task<bool> QCoroProcess::waitForFinished(std::chrono::milliseconds timeout) {
    const auto *process = qobject_cast<QProcess *>(mDevice.data());
    if (process->state() == QProcess::NotRunning) {
        co_return false;
    }

    const auto finished = co_await qCoro(process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), timeout);
    co_return finished.has_value();
}

QCoro::Task<bool> QCoroProcess::start(QIODevice::OpenMode mode, std::chrono::milliseconds timeout) {
    static_cast<QProcess *>(mDevice.data())->start(mode);
    return waitForStarted(timeout);
}

QCoro::Task<bool> QCoroProcess::start(const QString &program, const QStringList &arguments,
                                      QIODevice::OpenMode mode, std::chrono::milliseconds timeout) {
    static_cast<QProcess *>(mDevice.data())->start(program, arguments, mode);
    return waitForStarted(timeout);
}

#endif // QT_CONFIG(process)
