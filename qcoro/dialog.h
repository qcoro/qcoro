// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QPointer>
#include <QDialog>

namespace QCoro::detail
{

class DialogAwaiter {
public:
    explicit DialogAwaiter(QDialog *dialog)
        : mDialog(dialog)
    {}

    bool await_ready() const noexcept {
        return !mDialog || mDialog->isFinished();
    }

    void await_suspend(QCORO_STD::coroutine_handle<> awaitingCoroutine) {
        if (mReply) {
            QObject::connect(mReply, &QNetworkReply::finished,
                    [awaitingCoroutine]() mutable {
                        awaitingCoroutine.resume();
                    });
        } else {
            awaitingCoroutine.resume();
        }
    }

    QNetworkReply *await_resume() const {
        return mReply;
    }

private:
    QPointer<QNetworkReply> mReply;
};

template<>
struct awaiter_type<QNetworkReply *> {
    using type = NetworkReplyAwaiter;
};

} // namespace QCoro::detail
