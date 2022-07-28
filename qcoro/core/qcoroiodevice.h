// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "qcorotask.h"
#include "coroutine.h"
#include "macros_p.h"
#include "waitoperationbase_p.h"
#include "qcorocore_export.h"

#include <QPointer>

class QIODevice;

/*! \cond internal */

namespace QCoro::detail {

class QCOROCORE_EXPORT QCoroIODevice {
private:
    class OperationBase {
    public:
        Q_DISABLE_COPY(OperationBase)
        QCORO_DEFAULT_MOVE(OperationBase)

        virtual ~OperationBase() = default;

    protected:
        explicit OperationBase(QIODevice *device);

        virtual void finish(std::coroutine_handle<> awaitingCoroutine);

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
        virtual void await_suspend(std::coroutine_handle<> awaitingCoroutine) noexcept;
        QByteArray await_resume();

    private:
        std::function<QByteArray(QIODevice *)> mResultCb;
    };

    class ReadAllOperation final : public ReadOperation {
    public:
        explicit ReadAllOperation(QIODevice *device);
        explicit ReadAllOperation(QIODevice &device);
    };

    template<typename T>
    friend struct awaiter_type;
public:
    //! Constructor.
    explicit QCoroIODevice(QIODevice *device);

    /*!
     * \brief Co_awaitable equivalent to [`QIODevice::readAll()`][qdoc-qiodevice-readall].
     *
     * Waits until the `QIODevice` emits [`readyRead()`][qdoc-qiodevice-readyRead] and
     * then calls `readAll()`.
     *
     * Optional parameter timeout specifies how long to wait for the operation to
     * complete. If timeout occurs before any data are available for reading, the
     * operation will return an empty QByteArray. If the \c timeout is -1, the operation
     * will never time out.
     *
     * Identical to asynchronously calling
     * ```cpp
     * device.waitForReadyRead(timeout);
     * device.readAll();
     * ```
     *
     * [qdoc-qiodevice-readall]: https://doc.qt.io/qt-5/qiodevice.html#readAll
     * [qdoc-qiodevice-readyRead]: https://doc.qt.io/qt-5/qiodevice.html#readyRead
     */
    Task<QByteArray> readAll(std::chrono::milliseconds timeout = std::chrono::milliseconds{-1});

    /*!
     * \brief Co_awaitable equivalent to [`QIODevice::read()`][qdoc-qiodevice-read].
     *
     * Waits until the `QIODevice` emits [`readyRead()`][qdoc-qiodevice-readyRead] and
     * then calls [`read()`][qdoc-qiodevice-read] to read up to \c maxSize bytes.
     *
     * Optional parameter timeout specifies how long to wait for the operation to
     * complete. If timeout occurs before any data are available for reading, the
     * operation will return an empty QByteArray. If the timeout is -1, the operation
     * will never time out.
     *
     * Identical to asynchronously calling
     * ```cpp
     * device.waitForReadyRead(timeout);
     * device.read();
     * ```
     *
     * [qdoc-qiodevice-read]: https://doc.qt.io/qt-5/qiodevice.html#read-1
     * [qdoc-qiodevice-readyRead]: https://doc.qt.io/qt-5/qiodevice.html#readyRead
     */
    Task<QByteArray> read(qint64 maxSize,
                          std::chrono::milliseconds timeout = std::chrono::milliseconds{-1});

    /*!
     * \brief Co_awaitable equivalent to [`QIODevice::readLine()`][qdoc-qiodevice-readLine].
     *
     * Waits until the `QIODevice` emits [`readyRead()`][qdoc-qiodevice-readyRead] and
     * then calls [`readLine()`][qdoc-qiodevice-readLIne] to read until the end-of-line
     * character is reached or up to \c maxSize characters are read.
     *
     * Optional parameter timeout specifies how long to wait for the operation to
     * complete. If timeout occurs before any data are available for reading, the
     * operation will return an empty QByteArray. If the \c timeout is -1, the operation
     * will never time out.
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
    Task<QByteArray> readLine(qint64 maxSize = 0,
                              std::chrono::milliseconds timeout = std::chrono::milliseconds{-1});

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
    Task<qint64> write(const QByteArray &buffer);

    /*!
     * \brief Co_awaitable equivalent to [`QIODevice::waitForReadyRead`][qdoc-qiodevice-waitForReadyRead].
     *
     * Unlike the Qt version, this overload uses `std::chrono::milliseconds` to express the
     * timeout rather than plain `int`. If the \c timeout is -1, the operation will enver time out.
     *
     * [qdoc-qiodevice-waitForReadyRead]: https://doc.qt.io/qt-5/qiodevice.html#waitForReadyRead
     */
    Task<bool> waitForReadyRead(std::chrono::milliseconds timeout);

    /*!
     * \brief Co_awaitable equivalent to [`QIODevice::waitForReadyRead`][qdoc-qiodevice-waitForReadyRead].
     *
     * If the \c timeout_msecs is -1, the operation will never time out.
     *
     * [qdoc-qiodevice-waitForReadyRead]: https://doc.qt.io/qt-5/qiodevice.html#waitForReadyRead
     */
    Task<bool> waitForReadyRead(int timeout_msecs);

    /*!
     * \brief Co_awaitable equivalent to [`QIODevice::waitForBytesWritten`][qdoc-qiodevice-waitForBytesWritten].
     *
     * Unlike the Qt version, this overload uses `std::chrono::milliseconds` to express the
     * timeout rather than plain `int`. If the \c timeout is -1, the operation will never time out.
     *
     * [qdoc-qiodevice-waitForBytesWritten]: https://doc.qt.io/qt-5/qiodevice.html#waitForBytesWritten
     */
    Task<std::optional<qint64>> waitForBytesWritten(std::chrono::milliseconds timeout);

    /*!
     * \brief Co_awaitable equivalent to [`QIODevice::waitForBytesWritten`][qdoc-qiodevice-waitForBytesWritten].
     *
     * If the \c timeout_msecs is -1, the operation will never time out.
     *
     * [qdoc-qiodevice-waitForBytesWritten]: https://doc.qt.io/qt-5/qiodevice.html#waitForBytesWritten
     */
    Task<std::optional<qint64>> waitForBytesWritten(int timeout_msecs);

protected:
    virtual Task<std::optional<bool>> waitForReadyReadImpl(std::chrono::milliseconds timeout);
    virtual Task<std::optional<qint64>> waitForBytesWrittenImpl(std::chrono::milliseconds timeout);

    QPointer<QIODevice> mDevice = {};
};

template<typename T> requires std::is_base_of_v<QIODevice, T>
struct awaiter_type<T> {
    using type = QCoroIODevice::ReadAllOperation;
};
template<typename T> requires std::is_base_of_v<QIODevice, T>
struct awaiter_type<T *> {
    using type = QCoroIODevice::ReadAllOperation;
};

} // namespace QCoro::detail

/*! \endcond */

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

// If you got here due to a compile error, make sure to #include the QCoro header
// for the corresponding class, so that the qCoro() overload that wraps the QIODevice-derived
// classes in their respective QCoroIODevice-derived wrappers is used.
//
// Wrapping those classes directly into QCoroIODevice will cause co_awaiting certain operations to not
// work as expected.
class QAbstractSocket;
auto qCoro(QAbstractSocket *) noexcept; // You are likely missing "#include <QCoroAbstractSocket>"
class QLocalSocket;
auto qCoro(QLocalSocket *) noexcept; // You are likely missing "#include <QCoroLocalSocket>"
class QNetworkReply;
auto qCoro(QNetworkReply *) noexcept; // You are likely missing "#include <QNetworkReply>"
