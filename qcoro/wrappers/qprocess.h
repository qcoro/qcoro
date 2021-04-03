// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "qiodevice.h"
#include "../macros.h"

#include <QProcess>
#include <QPointer>

#include <chrono>

namespace QCoro::detail {

using namespace std::chrono_literals;

class QProcessStartOperation {
public:
    QProcessStartOperation(QProcess *process, QIODevice::OpenMode openMode)
        : mProcess{process} {
        connectFinished();
        mProcess->start(openMode);
    }

    QProcessStartOperation(QProcess *process, const QString &program, const QStringList &arguments,
                           QIODevice::OpenMode mode)
        : mProcess{process} {
        connectFinished();
        mProcess->start(program, arguments, mode);
    }

    Q_DISABLE_COPY(QProcessStartOperation);
    QCORO_DEFAULT_MOVE(QProcessStartOperation);

    ~QProcessStartOperation() = default;

    bool await_ready() const noexcept {
        return !mProcess || mProcess->state() == QProcess::Running || mFinished;
    }

    void await_suspend(QCORO_STD::coroutine_handle<> awaitingCoroutine) noexcept {
        mStartedConn = QObject::connect(mProcess, &QProcess::stateChanged,
            [this, awaitingCoroutine](auto newState) mutable {
                switch (newState) {
                    case QProcess::NotRunning:
                        // State changed from Starting or Running to NotRunning, which means
                        // there was some error. Wake up the coroutine.
                        QObject::disconnect(mStartedConn);
                        awaitingCoroutine.resume();
                        break;
                    case QProcess::Starting:
                        // Wait for it...
                        break;
                    case QProcess::Running:
                        QObject::disconnect(mStartedConn);
                        awaitingCoroutine.resume();
                        break;
                    }
            });
    }

    constexpr void await_resume() const noexcept {}

private:
    void connectFinished() {
        mFinishedConn = QObject::connect(mProcess, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
             [this]() {
                mFinished = true;
                QObject::disconnect(mFinishedConn);
            });
    }

    QPointer<QProcess> mProcess;
    QMetaObject::Connection mFinishedConn;
    QMetaObject::Connection mStartedConn;
    bool mFinished = false;
};

class QProcessWaitFinishedOperation {
public:
    QProcessWaitFinishedOperation(QProcess *process, std::chrono::milliseconds timeout)
        : mProcess(process), mTimeout(timeout)
    {}

    Q_DISABLE_COPY(QProcessWaitFinishedOperation);
    QCORO_DEFAULT_MOVE(QProcessWaitFinishedOperation);

    ~QProcessWaitFinishedOperation() = default;

    bool await_ready() const noexcept {
        return !mProcess || mProcess->state() == QProcess::NotRunning;
    }

    void await_suspend(QCORO_STD::coroutine_handle<> awaitingCoroutine) {
        mFinishedConn = QObject::connect(mProcess, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            [this, awaitingCoroutine]() mutable {
                QObject::disconnect(mFinishedConn);
                awaitingCoroutine.resume();
            });
        if (mTimeout > 0ms) {
            QTimer::singleShot(mTimeout, mProcess, [this, awaitingCoroutine]() mutable {
                QObject::disconnect(mFinishedConn);
                mTimedout = true;
                awaitingCoroutine.resume();
            });
        }
    }

    bool await_resume() noexcept {
        return !mTimedout;
    }

private:
    QPointer<QProcess> mProcess;
    QMetaObject::Connection mFinishedConn;
    std::chrono::milliseconds mTimeout = 30s;
    bool mTimedout = false;
};

class QProcessWrapper : public QIODeviceWrapper {
public:
    explicit QProcessWrapper(QProcess *process)
        : QIODeviceWrapper(process)
    {}

    auto start(QIODevice::OpenMode mode = QIODevice::ReadWrite) {
        return QProcessStartOperation{static_cast<QProcess *>(mDevice.data()), mode};
    }

    auto start(const QString &program, const QStringList &arguments,
               QIODevice::OpenMode mode = QIODevice::ReadWrite) {
        return QProcessStartOperation{static_cast<QProcess *>(mDevice.data()), program, arguments, mode};
    }

    auto finished(std::chrono::milliseconds timeout) {
        return QProcessWaitFinishedOperation{static_cast<QProcess *>(mDevice.data()), timeout};
    }

    auto finished(int timeout_msecs = 30000) {
        return finished(std::chrono::milliseconds{timeout_msecs});
    }

};

} // namespace QCoro::detail
