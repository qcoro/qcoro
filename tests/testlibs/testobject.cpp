// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "testobject.h"
#include <chrono>

using namespace QCoro;

TestContext::TestContext(QEventLoop &el) : mEventLoop(&el) {
    mEventLoop->setProperty("testFinished", false);
    mEventLoop->setProperty("shouldNotSuspend", false);
    mEventLoop->setProperty("expectTimeout", false);
}

TestContext::TestContext(TestContext &&other) noexcept {
    std::swap(mEventLoop, other.mEventLoop);
}

TestContext::~TestContext() {
    if (mEventLoop) {
        mEventLoop->setProperty("testFinished", true);
        mEventLoop->quit();
    }
}

TestContext &TestContext::operator=(TestContext &&other) noexcept {
    std::swap(mEventLoop, other.mEventLoop);
    return *this;
}

void TestContext::setShouldNotSuspend() {
    mEventLoop->setProperty("shouldNotSuspend", true);
}

void TestContext::setTimeout(std::chrono::milliseconds timeout) {
    auto *timer = qobject_cast<QTimer *>(mEventLoop->property("timeout").value<QObject *>());
    timer->start(timeout);
}

void TestContext::expectTimeout() {
    mEventLoop->setProperty("expectTimeout", true);
}