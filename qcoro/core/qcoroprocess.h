// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "waitoperationbase_p.h"
#include "qcoroiodevice.h"
#include "qcorocore_export.h"

#include <chrono>

#include <QIODevice>

#if QT_CONFIG(process)

class QProcess;

namespace QCoro::detail {

using namespace std::chrono_literals;

//! QProcess wrapper with co_awaitable-friendly API.
class QCOROCORE_EXPORT QCoroProcess : public QCoroIODevice {
public:
    explicit QCoroProcess(QProcess *process);

    /*!
     * \brief Co_awaitable equivalent to [`QProcess::waitForStarted()`][qtdoc-qprocess-waitForStarted].
     *
     * Returns true if the process has started successfully, otherwise returns false (if the
     * operation timed out or if an error occured).
     *
     * If \c timeout_msecs is -1 the operation will never time out.
     *
     * [qtdoc-qprocess-waitForStarted]: https://doc.qt.io/qt-5/qprocess.html#waitForStarted
     */
    Task<bool> waitForStarted(int timeout_msecs = 30'000);

    /*!
     * \brief Co_awaitable equivalent to [`QProcess::waitForStarted()`][qtdoc-qprocess-waitForStarted].
     *
     * Returns true if the process has started successfully, otherwise returns false (if the
     * operation timed out or if an error occured).
     *
     * Unlike the Qt version, this overload uses `std::chrono::milliseconds` to express the
     * timeout rather than plain `int`. If the \c timeout is -1 the operation will never time out.
     *
     * [qtdoc-qprocess-waitForStarted]: https://doc.qt.io/qt-5/qprocess.html#waitForStarted
     */
    Task<bool> waitForStarted(std::chrono::milliseconds timeout);

    /*!
     * \brief Co_awaitable equivalent to [`QProcess::waitForFinished()`][qtdoc-qprocess-waitForFinished].
     *
     * Returns true if the process has finished, otherwise returns false (if the operation timed
     * out, if an error occured or if this `QProcess` is already finished.
     *
     * If \c timeout_msecs is -1 the operation will never time out.
     *
     * [qtdoc-qprocess-waitForFinished]: https://doc.qt.io/qt-5/qprocess.html#waitForFinished
     */
    Task<bool> waitForFinished(int timeout_msecs = 30'000);

    /*!
     * \brief Co_awaitable equivalent to [`QProcess::waitForFinished()`][qtdoc-qprocess-waitForFinished].
     *
     * Returns true if the process has finished, otherwise returns false (if the operation timed
     * out, if an error occured or if this `QProcess` is already finished.
     *
     * Unlike the Qt version, this overload uses `std::chrono::milliseconds` to express the
     * timeout rather than plain `int`. If the \c timeout is -1 the operation will never time out.
     *
     * [qtdoc-qprocess-waitForFinished]: https://doc.qt.io/qt-4/qprocess.html#waitForFinished
     */
    Task<bool> waitForFinished(std::chrono::milliseconds timeout);

    /*!
     * \brief Executes a new process and waits for it to start
     *
     * Co_awaitable equivalent to calling [`QProcess::start()`][qtdoc-qprocess-start-2]
     * followed by [`QProcess::waitForStarted()`][qtdoc-qprocess-waitForStarted].
     *
     * Returns true if the process has started successfully, otherwise returns false (if the
     * operation timed out or if an error occurred).
     *
     * If the \c timeout is -1 the operation will never time out.
     *
     * [qtdoc-qprocess-waitForStarted]: https://doc.qt.io/qt-5/qprocess.html#waitForStarted
     * [qtdoc-qprocess-start-2]: https://doc.qt.io/qt-5/qprocess.html#start-2
     */
    Task<bool> start(QIODevice::OpenMode mode = QIODevice::ReadWrite,
                     std::chrono::milliseconds timeout = std::chrono::seconds(30));

    /*!
     * \brief Executes a new process and waits for it to start
     *
     * Co_awaitable equivalent to calling [`QProcess::start()`][qtdoc-qprocess-start]
     * followed by [`QProcess::waitForStarted()`][qtdoc-qprocess-waitForStarted].
     *
     * Returns true if the process has started successfully, otherwise returns false (if the
     * operation timed out or if an error occurred).
     *
     * If the \c timeout is -1 the operation will never time out.
     *
     * [qtdoc-qprocess-waitForStarted]: https://doc.qt.io/qt-5/qprocess.html#waitForStarted
     * [qtdoc-qprocess-start]: https://doc.qt.io/qt-5/qprocess.html#start-2
     */
    Task<bool> start(const QString &program, const QStringList &arguments,
                     QIODevice::OpenMode mode = QIODevice::ReadWrite,
                     std::chrono::milliseconds timeout = std::chrono::seconds(30));
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

#endif // QT_CONFIG(process)
