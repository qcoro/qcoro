// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "qcorotcpserver.h"
#include "qcorosignal.h"

#include <QTcpServer>

using namespace QCoro::detail;

QCoroTcpServer::WaitForNewConnectionOperation::WaitForNewConnectionOperation(QTcpServer *server, int timeout_msecs)
    : WaitOperationBase(server, timeout_msecs) {}

bool QCoroTcpServer::WaitForNewConnectionOperation::await_ready() const noexcept {
    return !mObj || mObj->hasPendingConnections();
}

void QCoroTcpServer::WaitForNewConnectionOperation::await_suspend(std::coroutine_handle<> awaitingCoroutine) noexcept {
    mConn = QObject::connect(mObj, &QTcpServer::newConnection,
                             std::bind(&WaitForNewConnectionOperation::resume, this, awaitingCoroutine));
    startTimeoutTimer(awaitingCoroutine);
}

QTcpSocket *QCoroTcpServer::WaitForNewConnectionOperation::await_resume() {
    return mTimedOut ? nullptr : mObj->nextPendingConnection();
}

QCoroTcpServer::QCoroTcpServer(QTcpServer *server)
    : mServer(server)
{}

QCoro::Task<QTcpSocket *> QCoroTcpServer::waitForNewConnection(int timeout_msecs) {
    return waitForNewConnection(std::chrono::milliseconds(timeout_msecs));
}

QCoro::Task<QTcpSocket *> QCoroTcpServer::waitForNewConnection(std::chrono::milliseconds timeout) {
    const auto server = mServer;
    if (!server->isListening()) {
        co_return nullptr;
    }
    if (server->hasPendingConnections()) {
        co_return server->nextPendingConnection();
    }

    const auto result = co_await qCoro(server.data(), &QTcpServer::newConnection, timeout);
    if (result.has_value()) {
        co_return server->nextPendingConnection();
    }
    co_return nullptr;
}
