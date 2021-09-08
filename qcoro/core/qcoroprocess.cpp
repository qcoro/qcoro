// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "qcoroprocess.h"

#include <QProcess>

using namespace QCoro::detail;

QCoroProcess::WaitForStartedOperation::WaitForStartedOperation(QProcess *process, int timeout_msecs)
    : WaitOperationBase(process, timeout_msecs)
{}

bool QCoroProcess::WaitForStartedOperation::await_ready() const noexcept {
    return !mObj || mObj->state() == QProcess::Running;
}

void QCoroProcess::WaitForStartedOperation::await_suspend(QCORO_STD::coroutine_handle<> awaitingCoroutine) noexcept {
    mConn = QObject::connect(mObj, &QProcess::stateChanged,
                             [this, awaitingCoroutine](auto newState) mutable {
                                 switch (newState) {
                                 case QProcess::NotRunning:
                                     // State changed from Starting or Running to NotRunning, which means
                                     // there was some error. Wake up the coroutine.
                                     resume(awaitingCoroutine);
                                     break;
                                 case QProcess::Starting:
                                     // Wait for it...
                                     break;
                                 case QProcess::Running:
                                     resume(awaitingCoroutine);
                                     break;
                                 }
                             });

    startTimeoutTimer(awaitingCoroutine);
}

QCoroProcess::WaitForFinishedOperation::WaitForFinishedOperation(QProcess *process, int timeout_msecs)
    : WaitOperationBase(process, timeout_msecs) {}

bool QCoroProcess::WaitForFinishedOperation::await_ready() const noexcept {
    return !mObj || mObj->state() == QProcess::NotRunning;
}

void QCoroProcess::WaitForFinishedOperation::await_suspend(QCORO_STD::coroutine_handle<> awaitingCoroutine) {
    mConn = QObject::connect(mObj, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
                             std::bind(&WaitForFinishedOperation::resume, this, awaitingCoroutine));
    startTimeoutTimer(awaitingCoroutine);
}

QCoroProcess::QCoroProcess(QProcess *process)
    : QCoroIODevice(process)
{}

QCoroProcess::WaitForStartedOperation QCoroProcess::waitForStarted(int timeout_msecs) {
    return WaitForStartedOperation{static_cast<QProcess *>(mDevice.data()), timeout_msecs};
}

QCoroProcess::WaitForStartedOperation QCoroProcess::waitForStarted(std::chrono::milliseconds timeout) {
    return waitForStarted(static_cast<int>(timeout.count()));
}

QCoroProcess::WaitForFinishedOperation QCoroProcess::waitForFinished(int timeout_msecs) {
    return WaitForFinishedOperation{static_cast<QProcess *>(mDevice.data()), timeout_msecs};
}

QCoroProcess::WaitForFinishedOperation QCoroProcess::waitForFinished(std::chrono::milliseconds timeout) {
    return waitForFinished(static_cast<int>(timeout.count()));
}

QCoroProcess::WaitForStartedOperation QCoroProcess::start(QIODevice::OpenMode mode) {
    static_cast<QProcess *>(mDevice.data())->start(mode);
    return waitForStarted();
}

QCoroProcess::WaitForStartedOperation QCoroProcess::start(const QString &program, const QStringList &arguments,
                                                          QIODevice::OpenMode mode) {
    static_cast<QProcess *>(mDevice.data())->start(program, arguments, mode);
    return waitForStarted();
}
