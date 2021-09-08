// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "waitoperationbase_p.h"
#include "qcoroiodevice.h"

#include <chrono>

#include <QIODevice>

class QProcess;

namespace QCoro::detail {

using namespace std::chrono_literals;

//! QProcess wrapper with co_awaitable-friendly API.
class QCoroProcess : public QCoroIODevice {
    //! An Awaitable that suspends the coroutine until the process is started.
    class WaitForStartedOperation : public WaitOperationBase<QProcess> {
    public:
        WaitForStartedOperation(QProcess *process, int timeout_msecs = 30'000);
        bool await_ready() const noexcept;
        void await_suspend(QCORO_STD::coroutine_handle<> awaitingCoroutine) noexcept;
    };

    //! An Awaitable that suspends the coroutine until the process is finished.
    class WaitForFinishedOperation : public WaitOperationBase<QProcess> {
    public:
        WaitForFinishedOperation(QProcess *process, int timeout_msecs);
        bool await_ready() const noexcept;
        void await_suspend(QCORO_STD::coroutine_handle<> awaitingCoroutine);
    };

public:
    explicit QCoroProcess(QProcess *process);

    /*!
     * \brief Co_awaitable equivalent to [`QProcess::waitForStarted()`][qtdoc-qprocess-waitForStarted].
     *
     * [qtdoc-qprocess-waitForStarted]: https://doc.qt.io/qt-5/qprocess.html#waitForStarted
     */
    WaitForStartedOperation waitForStarted(int timeout_msecs = 30'000);

    /*!
     * \brief Co_awaitable equivalent to [`QProcess::waitForStarted()`][qtdoc-qprocess-waitForStarted].
     *
     * Unlike the Qt version, this overload uses `std::chrono::milliseconds` to express the
     * timeout rather than plain `int`.
     *
     * [qtdoc-qprocess-waitForStarted]: https://doc.qt.io/qt-5/qprocess.html#waitForStarted
     */
    WaitForStartedOperation waitForStarted(std::chrono::milliseconds timeout);

    /*!
     * \brief Co_awaitable equivalent to [`QProcess::waitForFinished()`][qtdoc-qprocess-waitForFinished].
     *
     * [qtdoc-qprocess-waitForFinished]: https://doc.qt.io/qt-5/qprocess.html#waitForFinished
     */
    WaitForFinishedOperation waitForFinished(int timeout_msecs = 30'000);

    /*!
     * \brief Co_awaitable equivalent to [`QProcess::waitForFinished()`][qtdoc-qprocess-waitForFinished].
     *
     * Unlike the Qt version, this overload uses `std::chrono::milliseconds` to express the
     * timeout rather than plain `int`.
     *
     * [qtdoc-qprocess-waitForFinished]: https://doc.qt.io/qt-4/qprocess.html#waitForFinished
     */
    WaitForFinishedOperation waitForFinished(std::chrono::milliseconds timeout);

    /*!
     * \brief Executes a new process and waits for it to start
     *
     * Co_awaitable equivalent to calling [`QProcess::start()`][qtdoc-qprocess-start-2]
     * followed by [`QProcess::waitForStarted()`][qtdoc-qprocess-waitForStarted].
     *
     * [qtdoc-qprocess-waitForStarted]: https://doc.qt.io/qt-5/qprocess.html#waitForStarted
     * [qtdoc-qprocess-start-2]: https://doc.qt.io/qt-5/qprocess.html#start-2
     */
    WaitForStartedOperation start(QIODevice::OpenMode mode = QIODevice::ReadWrite);

    /*!
     * \brief Executes a new process and waits for it to start
     *
     * Co_awaitable equivalent to calling [`QProcess::start()`][qtdoc-qprocess-start]
     * followed by [`QProcess::waitForStarted()`][qtdoc-qprocess-waitForStarted].
     *
     * [qtdoc-qprocess-waitForStarted]: https://doc.qt.io/qt-5/qprocess.html#waitForStarted
     * [qtdoc-qprocess-start]: https://doc.qt.io/qt-5/qprocess.html#start-2
     */
    WaitForStartedOperation start(const QString &program, const QStringList &arguments,
                                  QIODevice::OpenMode mode = QIODevice::ReadWrite);
};

} // namespace QCoro::detail

//! Returns a coroutine-friendly wrapper for QProcess object.
/*!
 * Returns a wrapper for the QProcess \c p that provides coroutine-friendly
 * way to co_await the process to start or finish.
 *
 * @see docs/reference/qprocess.md
 */
inline auto qCoro(QProcess &p) noexcept {
    return QCoro::detail::QCoroProcess{&p};
}
//! \copydoc qCoro(QProcess &p) noexcept
inline auto qCoro(QProcess *p) noexcept {
    return QCoro::detail::QCoroProcess{p};
}

