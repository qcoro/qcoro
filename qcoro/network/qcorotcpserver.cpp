// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "qcorotcpserver.h"

#include <QTcpServer>

using namespace QCoro::detail;

QCoroTcpServer::WaitForNewConnectionOperation::WaitForNewConnectionOperation(QTcpServer *server, int timeout_msecs)
    : WaitOperationBase(server, timeout_msecs) {}

bool QCoroTcpServer::WaitForNewConnectionOperation::await_ready() const noexcept {
    return !mObj || mObj->hasPendingConnections();
}

void QCoroTcpServer::WaitForNewConnectionOperation::await_suspend(QCORO_STD::coroutine_handle<> awaitingCoroutine) noexcept {
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

QCoroTcpServer::WaitForNewConnectionOperation QCoroTcpServer::waitForNewConnection(int timeout_msecs) {
    return WaitForNewConnectionOperation{mServer.data(), timeout_msecs};
}

QCoroTcpServer::WaitForNewConnectionOperation QCoroTcpServer::waitForNewConnection(std::chrono::milliseconds timeout) {
    return WaitForNewConnectionOperation{mServer.data(), static_cast<int>(timeout.count())};
}

