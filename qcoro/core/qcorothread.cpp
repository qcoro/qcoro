// SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "qcorothread.h"
#include "qcorosignal.h"

#include <QThread>

using namespace QCoro::detail;

QCoroThread::QCoroThread(QThread *thread)
    : mThread(thread)
{}

QCoro::Task<bool> QCoroThread::waitForStarted(std::chrono::milliseconds timeout) {
    if (mThread->isRunning()) {
        co_return true;
    }
    if (mThread->isFinished()) {
        co_return false;
    }

    const auto result = co_await qCoro(mThread.data(), &QThread::started, timeout);
    co_return result.has_value();
}

QCoro::Task<bool> QCoroThread::waitForFinished(std::chrono::milliseconds timeout) {
    if (mThread->isFinished()) {
        co_return true;
    }
    if (!mThread->isRunning()) {
        co_return false;
    }

    const auto result = co_await qCoro(mThread.data(), &QThread::finished, timeout);
    co_return result.has_value();
}
