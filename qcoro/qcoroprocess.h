// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "qcoroiodevice.h"

#include <QProcess>
#include <QPointer>

#include <chrono>

namespace QCoro::detail {

using namespace std::chrono_literals;

//! QProcess wrapper with co_awaitable-friendly API.
class QCoroProcess : public QCoroIODevice {
    //! Base class for co_awaitable waitFor* operations.
    class WaitOperationBase {
    public:
        Q_DISABLE_COPY(WaitOperationBase)
        QCORO_DEFAULT_MOVE(WaitOperationBase)

        ~WaitOperationBase() = default;

        bool await_resume() noexcept {
            return !mTimedOut;
        }

    protected:
        WaitOperationBase(QProcess *process, int timeout_msecs)
            : mProcess{process}
        {
            if (timeout_msecs > -1) {
                mTimeoutTimer = std::make_unique<QTimer>();
                mTimeoutTimer->setInterval(timeout_msecs);
                mTimeoutTimer->setSingleShot(true);
            }
        }

        void startTimeoutTimer(QCORO_STD::coroutine_handle<> awaitingCoroutine) {
            if (!mTimeoutTimer) {
                return;
            }

            QObject::connect(mTimeoutTimer.get(), &QTimer::timeout,
                [this, awaitingCoroutine]() mutable {
                    mTimedOut = true;
                    resume(awaitingCoroutine);
                });
            mTimeoutTimer->start();
        }

        void resume(QCORO_STD::coroutine_handle<> awaitingCoroutine) {
            if (mTimeoutTimer) {
                mTimeoutTimer->stop();
            }

            QObject::disconnect(mConn);

            awaitingCoroutine.resume();
        }

        QPointer<QProcess> mProcess;
        std::unique_ptr<QTimer> mTimeoutTimer;
        QMetaObject::Connection mConn;
        bool mTimedOut = false;
    };

    //! An Awaitable that suspends the coroutine until the process is started.
    class WaitForStartedOperation : public WaitOperationBase {
    public:
        WaitForStartedOperation(QProcess *process, int timeout_msecs = 30'000)
            : WaitOperationBase(process, timeout_msecs) {}

        bool await_ready() const noexcept {
            return !mProcess || mProcess->state() == QProcess::Running;
        }

        void await_suspend(QCORO_STD::coroutine_handle<> awaitingCoroutine) noexcept {
            mConn = QObject::connect(mProcess, &QProcess::stateChanged,
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
    };

    //! An Awaitable that suspends the coroutine until the process is finished.
    class WaitForFinishedOperation : public WaitOperationBase {
    public:
        WaitForFinishedOperation(QProcess *process, int timeout_msecs)
            : WaitOperationBase(process, timeout_msecs) {}

        bool await_ready() const noexcept {
            return !mProcess || mProcess->state() == QProcess::NotRunning;
        }

        void await_suspend(QCORO_STD::coroutine_handle<> awaitingCoroutine) {
            mConn = QObject::connect(mProcess, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
                [this, awaitingCoroutine]() mutable {
                    resume(awaitingCoroutine);
                });
            startTimeoutTimer(awaitingCoroutine);
        }
    };

public:
    explicit QCoroProcess(QProcess *process)
        : QCoroIODevice(process)
    {}

    //! Co_awaitable equivalent  to `[QProcess::waitForStarted()][qtdoc-qprocess-waitForStarted]`.
    Awaitable auto waitForStarted(int timeout_msecs = 30'000) {
        return WaitForStartedOperation{static_cast<QProcess *>(mDevice.data()), timeout_msecs};
    }
    //
    //! Co_awaitable equivalent to `[QProcess::waitForStarted()][qtdoc-qprocess-waitForStarted]`.
    /*!
     * Unlike the Qt version, this overload uses `std::chrono::milliseconds` to express the
     * timeout rather than plain `int`.
     */
    Awaitable auto waitForStarted(std::chrono::milliseconds timeout) {
        return waitForStarted(static_cast<int>(timeout.count()));
    }

    //! Co_awaitable equivalent to `[QProcess::waitForFinished()][qtdoc-qprocess-waitForFinished]`.
    Awaitable auto waitForFinished(int timeout_msecs = 30'000) {
        return WaitForFinishedOperation{static_cast<QProcess *>(mDevice.data()), timeout_msecs};
    }

    //! Co_awaitable equivalent to `[QProcess::waitForFinished()][qtdoc-qprocess-waitForFinished]`.
    /*!
     * Unlike the Qt version, this overload uses `std::chrono::milliseconds` to express the
     * timeout rather than plain `int`.
     */
    Awaitable auto waitForFinished(std::chrono::milliseconds timeout) {
        return waitForFinished(static_cast<int>(timeout.count()));
    }

    //! Executes a new process and waits for it to start
    /*!
     * Co_awaitable equivalent to calling `[QProcess::start()][qtdoc-qprocess-start-2]`
     * followed by `[QProcess::waitForStarted()][qtdoc-qprocess-waitForStarted]`.
     */
    Awaitable auto start(QIODevice::OpenMode mode = QIODevice::ReadWrite) {
        static_cast<QProcess *>(mDevice.data())->start(mode);
        return waitForStarted();
    }

    //! Executes a new process and waits for it to start
    /*!
     * Co_awaitable equivalent to calling `[QProcess::start()][qtdoc-qprocess-start]`
     * followed by `[QProcess::waitForStarted()][qtdoc-qprocess-waitForStarted]`.
     */
    Awaitable auto start(const QString &program, const QStringList &arguments,
               QIODevice::OpenMode mode = QIODevice::ReadWrite) {
        static_cast<QProcess *>(mDevice.data())->start(program, arguments, mode);
        return waitForStarted();
    }

};

} // namespace QCoro::detail

/*!
 * [qtdoc-qprocess-start]: https://doc.qt.io/qt-5/qprocess.html#start
 * [qtdoc-qprocess-start-2]: https://doc.qt.io/qt-5/qprocess.html#start-2
 * [qtdoc-qprocess-waitForStarted]: https://doc.qt.io/qt-5/qprocess.html#waitForStarted
 * [qtdoc-qprocess-waitForFinished]: https://doc.qt.io/qt-5/qprocess.html#waitForFinished
 */

