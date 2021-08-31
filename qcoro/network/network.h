// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "qcoro/task.h"

#include <QPointer>

class QNetworkReply;

/*! \cond internal */

namespace QCoro::detail {

class NetworkReplyAwaiter {
public:
    explicit NetworkReplyAwaiter(QNetworkReply *reply);

    bool await_ready() const noexcept;
    void await_suspend(QCORO_STD::coroutine_handle<> awaitingCoroutine);
    QNetworkReply *await_resume() const;

private:
    QPointer<QNetworkReply> mReply;
};

template<>
struct awaiter_type<QNetworkReply *> {
    using type = NetworkReplyAwaiter;
};

} // namespace QCoro::detail

/*! \endcond */
