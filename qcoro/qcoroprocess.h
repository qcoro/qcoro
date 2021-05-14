// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "impl/waitoperationbase.h"
#include "qcoroiodevice.h"

#include <QPointer>
#include <QProcess>

#include <chrono>

namespace QCoro::detail {

using namespace std::chrono_literals;

//! QProcess wrapper with co_awaitable-friendly API.
class QCoroProcess : public QCoroIODevice {
    //! An Awaitable that suspends the coroutine until the process is started.
    class WaitForStartedOperation : public WaitOperationBase<QProcess> {
    public:
        WaitForStartedOperation(QProcess *process, int timeout_msecs = 30'000)
            : WaitOperationBase(process, timeout_msecs) {}

        bool await_ready() const noexcept {
            return !mObj || mObj->state() == QProcess::Running;
        }

        void await_suspend(QCORO_STD::coroutine_handle<> awaitingCoroutine) noexcept {
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
    };

    //! An Awaitable that suspends the coroutine until the process is finished.
    class WaitForFinishedOperation : public WaitOperationBase<QProcess> {
    public:
        WaitForFinishedOperation(QProcess *process, int timeout_msecs)
            : WaitOperationBase(process, timeout_msecs) {}

        bool await_ready() const noexcept {
            return !mObj || mObj->state() == QProcess::NotRunning;
        }

        void await_suspend(QCORO_STD::coroutine_handle<> awaitingCoroutine) {
            mConn = QObject::connect(
                mObj, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
                [this, awaitingCoroutine]() mutable { resume(awaitingCoroutine); });
            startTimeoutTimer(awaitingCoroutine);
        }
    };

public:
    explicit QCoroProcess(QProcess *process) : QCoroIODevice(process) {}

    /*!
     * \brief Co_awaitable equivalent to [`QProcess::waitForStarted()`][qtdoc-qprocess-waitForStarted].
     *
     * [qtdoc-qprocess-waitForStarted]: https://doc.qt.io/qt-5/qprocess.html#waitForStarted
     */
    Awaitable auto waitForStarted(int timeout_msecs = 30'000) {
        return WaitForStartedOperation{static_cast<QProcess *>(mDevice.data()), timeout_msecs};
    }

    /*!
     * \brief Co_awaitable equivalent to [`QProcess::waitForStarted()`][qtdoc-qprocess-waitForStarted].
     *
     * Unlike the Qt version, this overload uses `std::chrono::milliseconds` to express the
     * timeout rather than plain `int`.
     *
     * [qtdoc-qprocess-waitForStarted]: https://doc.qt.io/qt-5/qprocess.html#waitForStarted
     */
    Awaitable auto waitForStarted(std::chrono::milliseconds timeout) {
        return waitForStarted(static_cast<int>(timeout.count()));
    }

    /*!
     * \brief Co_awaitable equivalent to [`QProcess::waitForFinished()`][qtdoc-qprocess-waitForFinished].
     *
     * [qtdoc-qprocess-waitForFinished]: https://doc.qt.io/qt-5/qprocess.html#waitForFinished
     */
    Awaitable auto waitForFinished(int timeout_msecs = 30'000) {
        return WaitForFinishedOperation{static_cast<QProcess *>(mDevice.data()), timeout_msecs};
    }

    /*!
     * \brief Co_awaitable equivalent to [`QProcess::waitForFinished()`][qtdoc-qprocess-waitForFinished].
     *
     * Unlike the Qt version, this overload uses `std::chrono::milliseconds` to express the
     * timeout rather than plain `int`.
     *
     * [qtdoc-qprocess-waitForFinished]: https://doc.qt.io/qt-4/qprocess.html#waitForFinished
     */
    Awaitable auto waitForFinished(std::chrono::milliseconds timeout) {
        return waitForFinished(static_cast<int>(timeout.count()));
    }

    /*!
     * \brief Executes a new process and waits for it to start
     *
     * Co_awaitable equivalent to calling [`QProcess::start()`][qtdoc-qprocess-start-2]
     * followed by [`QProcess::waitForStarted()`][qtdoc-qprocess-waitForStarted].
     *
     * [qtdoc-qprocess-waitForStarted]: https://doc.qt.io/qt-5/qprocess.html#waitForStarted
     * [qtdoc-qprocess-start-2]: https://doc.qt.io/qt-5/qprocess.html#start-2
     */
    Awaitable auto start(QIODevice::OpenMode mode = QIODevice::ReadWrite) {
        static_cast<QProcess *>(mDevice.data())->start(mode);
        return waitForStarted();
    }

    /*!
     * \brief Executes a new process and waits for it to start
     *
     * Co_awaitable equivalent to calling [`QProcess::start()`][qtdoc-qprocess-start]
     * followed by [`QProcess::waitForStarted()`][qtdoc-qprocess-waitForStarted].
     *
     * [qtdoc-qprocess-waitForStarted]: https://doc.qt.io/qt-5/qprocess.html#waitForStarted
     * [qtdoc-qprocess-start]: https://doc.qt.io/qt-5/qprocess.html#start-2
     */
    Awaitable auto start(const QString &program, const QStringList &arguments,
                         QIODevice::OpenMode mode = QIODevice::ReadWrite) {
        static_cast<QProcess *>(mDevice.data())->start(program, arguments, mode);
        return waitForStarted();
    }
};

} // namespace QCoro::detail
