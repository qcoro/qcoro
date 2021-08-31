// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "qcoro/task.h"
#include "qcoro/core/qcoroiodevice.h"
#include "qcoro/waitoperationbase_p.h"

#include <QNetworkReply>

namespace QCoro::detail {

class QCoroNetworkReply final : private QCoroIODevice {
private:
    class ReadOperation final : public QCoroIODevice::ReadOperation {
    public:
        using QCoroIODevice::ReadOperation::ReadOperation;

        bool await_ready() const noexcept final;
        void await_suspend(QCORO_STD::coroutine_handle<> awaitingCoroutine) noexcept final;

    private:
        void finish(QCORO_STD::coroutine_handle<> awaitingCoroutine) final;

        QMetaObject::Connection mFinishedConn;
    };

    class WaitForFinishedOperation final {
    public:
        explicit WaitForFinishedOperation(QPointer<QNetworkReply> reply);

        bool await_ready() const noexcept;
        void await_suspend(QCORO_STD::coroutine_handle<> awaitingCoroutine);
        QNetworkReply *await_resume() const noexcept;

    private:
        QPointer<QNetworkReply> mReply;
    };

    friend struct awaiter_type<QNetworkReply *>;
public:
    using QCoroIODevice::QCoroIODevice;

    ReadOperation readAll();
    ReadOperation read(qint64 maxSize);
    ReadOperation readLine(qint64 maxSize = 0);
    WaitForFinishedOperation waitForFinished();
};

} // namespace QCoro::detail

//! Returns a coroutine-friendly wrapper for QNetworkReply object.
/*!
 * Returns a wrapper for the QNetworkReply \c s that provides coroutine-friendly
 * way to co_await read and write operations.
 *
 * @see docs/reference/qnetworkreply.md
 */
inline auto qCoro(QNetworkReply &s) noexcept {
    return QCoro::detail::QCoroNetworkReply{&s};
}
//! \copydoc qCoro(QAbstractSocket &s) noexcept
inline auto qCoro(QNetworkReply *s) noexcept {
    return QCoro::detail::QCoroNetworkReply{s};
}

/*! \cond internal */
namespace QCoro::detail {

template<>
struct awaiter_type<QNetworkReply *> {
    using type = QCoroNetworkReply::WaitForFinishedOperation;
};

} // namespace QCoro::detail


