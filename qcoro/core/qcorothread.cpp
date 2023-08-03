// SPDX-FileCopyrightText: 2022 Daniel Vrátil <dvratil@kde.org>
//
// SPDX-License-Identifier: MIT

#include "qcorothread.h"
#include "qcorosignal.h"

#include <QThread>
#include <QEvent>
#include <QCoreApplication>

using namespace QCoro;
using namespace QCoro::detail;

namespace QCoro::detail {

class ContextHelper : public QObject {
    Q_OBJECT
public:
    static QEvent::Type eventType;

    explicit ContextHelper(std::coroutine_handle<> awaiter, QThread *thread)
        : mThread(thread)
        , mAwaiter(awaiter)
    {}

    bool event(QEvent *event) override {
        if (event->type() == eventType) {
            Q_ASSERT(QThread::currentThread() == mThread);
            mAwaiter.resume();
            return true;
        }

        return QObject::event(event);
    }

private:
    QThread *mThread;
    std::coroutine_handle<> mAwaiter;
};

QEvent::Type ContextHelper::eventType = static_cast<QEvent::Type>(QEvent::registerEventType());

class ThreadContextPrivate {
public:
    explicit ThreadContextPrivate(QThread *thread)
        : mThread(thread)
    {}

    QThread *mThread;
    std::unique_ptr<ContextHelper> mContext;
};

} // namespace QCoro::detail

ThreadContext::ThreadContext(QThread *thread)
    : d(std::make_unique<ThreadContextPrivate>(thread))
{}

#ifdef Q_CC_GNU
ThreadContext::ThreadContext(ThreadContext &&) noexcept  = default;
#endif

ThreadContext::~ThreadContext() = default;

bool ThreadContext::await_ready() const noexcept {
    return false; // never ready!
}

void ThreadContext::await_suspend(std::coroutine_handle<> awaiter) noexcept {
    d->mContext = std::make_unique<ContextHelper>(awaiter, d->mThread);
    d->mContext->moveToThread(d->mThread);
    qCoro(d->mThread).waitForStarted().then([this]() {
        auto *event = new QEvent(static_cast<QEvent::Type>(detail::ContextHelper::eventType));
        QCoreApplication::postEvent(d->mContext.get(), event);
    });
}

void ThreadContext::await_resume() noexcept {}


ThreadContext QCoro::moveToThread(QThread *thread) {
    return ThreadContext(thread);
}

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

#include "qcorothread.moc"
