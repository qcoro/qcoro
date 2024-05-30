// SPDX-FileCopyrightText: 2024 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "qcorosocketnotifier.h"
#include "qcorosignal.h"

using namespace QCoro;
using namespace QCoro::detail;

QCoroSocketNotifier::QCoroSocketNotifier(QSocketNotifier *notifier)
    : mNotifier(notifier)
{}

Task<bool> QCoroSocketNotifier::waitForActivated(std::chrono::milliseconds timeout)
{
    co_return co_await WaitForSocketActivatedOperation{mNotifier, timeout};
}

QCoroSocketNotifier::WaitForSocketActivatedOperation::WaitForSocketActivatedOperation(QSocketNotifier *notifier, std::chrono::milliseconds timeout)
    : mNotifier(notifier)
    , mTimeout(timeout)
{}

QCoroSocketNotifier::WaitForSocketActivatedOperation::WaitForSocketActivatedOperation(QSocketNotifier &notifier, std::chrono::milliseconds timeout)
    : WaitForSocketActivatedOperation(&notifier, timeout)
{}

QCoroSocketNotifier::WaitForSocketActivatedOperation::WaitForSocketActivatedOperation(QSocketNotifier &&notifier, std::chrono::milliseconds timeout)
    : WaitForSocketActivatedOperation(&notifier, timeout)
{}

bool QCoroSocketNotifier::WaitForSocketActivatedOperation::await_ready() const noexcept
{
    return !mNotifier || !mNotifier->isEnabled()
    #if QT_VERSION >= QT_VERSION_CHECK(6, 1, 0)
        || !mNotifier->isValid()
    #endif
    ;
}

void QCoroSocketNotifier::WaitForSocketActivatedOperation::await_suspend(std::coroutine_handle<> awaitingCoroutine)
{
    qCoro(mNotifier.data(), &QSocketNotifier::activated, mTimeout).then([awaitingCoroutine, this](const auto &result) mutable {
        mResult = result.has_value();
        awaitingCoroutine.resume();
    });
}

bool QCoroSocketNotifier::WaitForSocketActivatedOperation::await_resume() const noexcept
{
    return mResult;
}
