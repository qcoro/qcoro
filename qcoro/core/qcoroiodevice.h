// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "coroutine.h"
#include "macros_p.h"

#include <QPointer>

class QIODevice;

namespace QCoro::detail {

class QCoroIODevice {
private:
    class OperationBase {
    public:
        Q_DISABLE_COPY(OperationBase)
        QCORO_DEFAULT_MOVE(OperationBase)

        virtual ~OperationBase() = default;

    protected:
        explicit OperationBase(QIODevice *device);

        virtual void finish(QCORO_STD::coroutine_handle<> awaitingCoroutine);

        QPointer<QIODevice> mDevice;
        QMetaObject::Connection mConn;
        QMetaObject::Connection mCloseConn;
        QMetaObject::Connection mFinishedConn;
    };

protected:
    class ReadOperation : public OperationBase {
    public:
        ReadOperation(QIODevice *device, std::function<QByteArray(QIODevice *)> &&resultCb);
        Q_DISABLE_COPY(ReadOperation)
        QCORO_DEFAULT_MOVE(ReadOperation)

        virtual bool await_ready() const noexcept;
        virtual void await_suspend(QCORO_STD::coroutine_handle<> awaitingCoroutine) noexcept;
        QByteArray await_resume();

    private:
        std::function<QByteArray(QIODevice *)> mResultCb;
    };

    class WriteOperation : public OperationBase {
    public:
        WriteOperation(QIODevice *device, const QByteArray &data);
        Q_DISABLE_COPY(WriteOperation)
        QCORO_DEFAULT_MOVE(WriteOperation)

        bool await_ready() const noexcept;
        void await_suspend(QCORO_STD::coroutine_handle<> awaitingCoroutine) noexcept;
        qint64 await_resume() noexcept;

    private:
        qint64 mBytesToBeWritten = 0;
        qint64 mBytesWritten = 0;
    };

public:
    //! Constructor.
    explicit QCoroIODevice(QIODevice *device);

    /*!
     * \brief Co_awaitable equivalent to [`QIODevice::readAll()`][qdoc-qiodevice-readall].
     *
     * Waits until the `QIODevice` emits [`readyRead()`][qdoc-qiodevice-readyRead] and
     * then calls `readAll()`.
     *
     * Identical to asynchronously calling
     * ```cpp
     * device.waitForReadyRead();
     * device.readAll();
     * ```
     *
     * [qdoc-qiodevice-readall]: https://doc.qt.io/qt-5/qiodevice.html#readAll
     * [qdoc-qiodevice-readyRead]: https://doc.qt.io/qt-5/qiodevice.html#readyRead
     */
    ReadOperation readAll();

    /*!
     * \brief Co_awaitable equivalent to [`QIODevice::read()`][qdoc-qiodevice-read].
     *
     * Waits until the `QIODevice` emits [`readyRead()`][qdoc-qiodevice-readyRead] and
     * then calls [`read()`][qdoc-qiodevice-read] to read up to \c maxSize bytes.
     *
     * Identical to asynchronously calling
     * ```cpp
     * device.waitForReadyRead();
     * device.read();
     * ```
     *
     * [qdoc-qiodevice-read]: https://doc.qt.io/qt-5/qiodevice.html#read-1
     * [qdoc-qiodevice-readyRead]: https://doc.qt.io/qt-5/qiodevice.html#readyRead
     */
    ReadOperation read(qint64 maxSize);

    /*!
     * \brief Co_awaitable equivalent to [`QIODevice::readLine()`][qdoc-qiodevice-readLine].
     *
     * Waits until the `QIODevice` emits [`readyRead()`][qdoc-qiodevice-readyRead] and
     * then calls [`readLine()`][qdoc-qiodevice-readLIne] to read until the end-of-line
     * characte is reached or up to \c maxSize characters are read.
     *
     * Identical to asynchronously calling
     * ```cpp
     * device.waitForReadyRead();
     * device.readLine();
     * ```
     *
     * [qdoc-qiodevice-readLine]: https://doc.qt.io/qt-5/qiodevice.html#readLine
     * [qdoc-qiodevice-readyRead]: https://doc.qt.io/qt-5/qiodevice.html#readyRead
     */
    ReadOperation readLine(qint64 maxSize = 0);

    // TODO
    //auto bytesAvailable(qint64 minBytes) {

    /*!
     * \brief Co_awaitable equivalent to [`QIODevice::write`][qdoc-qiodevice-write].
     *
     * Returns immediately if the `QIODevice` is unbuffered, blocks until the `QIODevice`
     * emits [`bytesWritten()`][qdoc-qiodevice-bytesWritten] signal with total bytes equal
     * to the size of the input \c buffer.
     *
     * Identical to asynchronously calling
     * ```cpp
     * device.write(data);
     * device.waitForBytesWritten();
     * ```
     *
     * [qdoc-qiodevice-write]: https://doc.qt.io/qt-5/qiodevice.html#write-2
     * [qdoc-qiodevice-bytesWritten]: https://doc.qt.io/qt-5/qiodevice.html#bytesWritten
     */
    WriteOperation write(const QByteArray &buffer);

protected:
    QPointer<QIODevice> mDevice = {};
};

} // namespace QCoro::detail


//! Returns a coroutine-friendly wrapper for a QIODevice-derived object.
/*!
 * Returns a wrapper for QIODevice \c d that provides coroutine-friendly way
 * of co_awaiting reading and writing operation.
 *
 * @see docs/reference/qiodevice.md
 */
inline auto qCoro(QIODevice &d) noexcept {
    return QCoro::detail::QCoroIODevice{&d};
}
//! \copydoc qCoro(QIODevice *d) noexcept
inline auto qCoro(QIODevice *d) noexcept {
    return QCoro::detail::QCoroIODevice{d};
}


