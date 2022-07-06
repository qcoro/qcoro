// SPDX-FileCopyrightText: 2021 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "testobject.h"

using namespace QCoro;

TestContext::TestContext(QEventLoop &el) : mEventLoop(&el) {
    mEventLoop->setProperty("testFinished", false);
    mEventLoop->setProperty("shouldNotSuspend", false);
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
