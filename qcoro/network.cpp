// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "network.h"

#include <QNetworkReply>


using namespace QCoro::detail;

NetworkReplyAwaiter::NetworkReplyAwaiter(QNetworkReply *reply)
    : mReply(reply)
{}

bool NetworkReplyAwaiter::await_ready() const noexcept {
    return !mReply || mReply->isFinished();
}

void NetworkReplyAwaiter::await_suspend(QCORO_STD::coroutine_handle<> awaitingCoroutine) {
    if (mReply) {
        QObject::connect(mReply, &QNetworkReply::finished,
                         [awaitingCoroutine]() mutable { awaitingCoroutine.resume(); });
    } else {
        awaitingCoroutine.resume();
    }
}

QNetworkReply *NetworkReplyAwaiter::await_resume() const {
    return mReply;
}

